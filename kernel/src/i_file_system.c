#include "../include/i_file_system.h"

t_list* archivos_abiertos;
int file_id = 0;

void procesar_file_system(void) {

	iniciar_tablas_archivos_abiertos();

	while (true) {
		log_info(logger, "FS_THREAD -> esperando wait de request de archivo");
		sem_wait(&request_file_system);
	    // Obtener los datos necesarios del PCB para la solicitud
		t_pcb* pcb = (t_pcb*)squeue_pop(colas_planificacion->cola_archivos);
	    int pid = pcb->pid;
	    char* nombre_archivo = obtener_nombre_archivo(pcb);
	    log_info(logger, "FS_THREAD -> Request de pid %d para el archivo %s",pid, nombre_archivo);
	    t_archivo_abierto* archivo;
	    t_instruccion* instruccion = obtener_instruccion(pcb);
	    log_info(logger, "FS_THREAD -> Recibido PID con PC en %d", pcb->program_counter);
	    log_info(logger, "FS_THREAD -> Llamando a FS por la instrucción -> %d: %s", instruccion->codigo, nombre_de_instruccion(instruccion->codigo));

	    enviar_request_fs(pid, instruccion, nombre_archivo);
	    int cod_respuesta = recibir_entero(socket_filesystem);
	    log_info(logger, "FS_THREAD -> Recibida respuesta de FILESYSTEM -> %d", cod_respuesta);
	    switch (cod_respuesta) {
			case F_NOT_EXISTS:
				log_info(logger, "FS_THREAD -> El Archivo que se intentó abrir no existe. Enviando F_CREATE %s", nombre_archivo);
				archivo = fs_crear_archivo(nombre_archivo);

				enviar_request_fs(pid, instruccion, nombre_archivo);
				log_info(logger, "FS_THREAD -> Se creó el archivo %s en el path: %s", archivo->nombre);
				break;
			case F_OPEN_OK:
				archivo = obtener_archivo_abierto(nombre_archivo);
				if (archivo == NULL) {
					log_error(logger, "FS_THREAD -> ERROR: archivo is null");
				}
				log_info(logger, "FS_THREAD -> Abriendo archivo -> %s", archivo->nombre);
				list_add(archivos_abiertos, archivo);
				squeue_push(archivo->cola_bloqueados, pid);
				archivo->cant_aperturas++;
				break;
			default:
				break;
		}
	}
}

void enviar_request_fs(int pid, t_instruccion* instruccion, char* nombre_archivo) {
	switch (instruccion->codigo) {
		case ci_F_CLOSE:
			ejecutar_f_close(pid, nombre_archivo);
			break;
		case ci_F_SEEK:
			ejectuar_f_seek(pid, nombre_archivo, instruccion);
		case ci_F_OPEN:
			log_info(logger, "Enviando Request de ci_F_OPEN para el archivo %s ", nombre_archivo);
			enviar_entero(socket_filesystem, F_OPEN); // f_open ARCHIVO
			enviar_mensaje(nombre_archivo, socket_filesystem, logger);

			break;
		case ci_F_READ:
			log_info(logger, "Enviando Request de ci_F_READ para el archivo %s ", nombre_archivo);
			enviar_entero(socket_filesystem, F_READ);
			enviar_mensaje(nombre_archivo, socket_filesystem, logger);
			enviar_mensaje(list_get(instruccion->parametros,1), socket_filesystem, logger);
			enviar_mensaje(list_get(instruccion->parametros,2), socket_filesystem, logger);
			break;
		case ci_F_WRITE:
			log_info(logger, "Enviando Request de ci_F_WRITE para el archivo %s ", nombre_archivo);
			enviar_entero(socket_filesystem, F_WRITE);
			enviar_mensaje(nombre_archivo, socket_filesystem, logger);
			enviar_mensaje(list_get(instruccion->parametros,1), socket_filesystem, logger);
			enviar_mensaje(list_get(instruccion->parametros,2), socket_filesystem, logger);
			break;
		case ci_F_TRUNCATE:
			log_info(logger, "Enviando Request de ci_F_TRUNCATE para el archivo %s ", nombre_archivo);
			enviar_entero(socket_filesystem, F_TRUNCATE);
			enviar_mensaje(nombre_archivo, socket_filesystem, logger);
			enviar_mensaje(list_get(instruccion->parametros,1), socket_filesystem, logger);
			break;
		default:
			break;
	}
}

void ejectuar_f_seek(int pid, char* nombre_archivo, t_instruccion* instruccion) {
	t_archivo_abierto* archivo = obtener_archivo_abierto(nombre_archivo);
	if (archivo == NULL) {
		log_error(logger, "FS_THREAD -> ERROR: No existe el archivo %s entre los archivos abiertos", nombre_archivo);
		return;
	}
	int posicion_puntero = atoi((char*)list_get(instruccion, 1));
	log_info(logger, "FS_THREAD -> F_SEEK: Moviendo Puntero a la posición %d", posicion_puntero);
	pthread_mutex_lock(archivo->mutex);
	archivo->puntero = posicion_puntero;
	pthread_mutex_unlock(archivo->mutex);

	//TODO: FORZAR EJECUCIÓN
	sem_post(&f_seek_done);
}

void ejecutar_f_close(int pid, char* nombre_archivo) {
	t_archivo_abierto* archivo = obtener_archivo_abierto(nombre_archivo);

	if (archivo == NULL) {
		log_error(logger, "FS_THREAD -> ERROR: No existe el archivo %s entre los archivos abiertos", nombre_archivo);
		return;
	} else {
		// SI SOLO ESTE PID TIENE ABIERTO EL ARCHIVO
		if (queue_size(archivo->cola_bloqueados->cola) <= 1) {
			log_info(logger, "FS_THREAD -> Eliminando entrada en archivo %s para PID %d", nombre_archivo, pid);
			archivo_abierto_destroy(archivo);
			list_remove_element(archivos_abiertos, archivo);
		}
		// SI OTROS PROCESOS ESTAN BLOQUEADOS POR ESTE ARCHIVO -> SE DESBLOQUEA EL PRIMERO
		else
		{
			int pid_desbloqueado = squeue_pop(archivo->cola_bloqueados);
			log_info(logger, "FS_THREAD -> Desbloqueando PID %d por F_CLOSE del PID %d", pid_desbloqueado, pid);
			// TODO: ALGO CON ESTE PID
		}

	}
	sem_post(&f_close_done);
}

void hay_procesos_esperando_por_este_archivo(char *nombre_archivo) {

}

void desbloquear_el_primero(void) {

}

void eliminar_entrada_de_tabla_de_registros(void) {

}

t_instruccion* obtener_instruccion(t_pcb* pcb) {
	return (t_instruccion*)list_get(pcb->instrucciones, pcb->program_counter -1 ); // DEVUELVE LA INSTRUCCION ANTERIOR YA QUE EL CPU AVANZA EL PC SIEMPRE
}

t_archivo_abierto* fs_crear_archivo(char* nombre_archivo) {
	 // Enviar mensaje al módulo de file system para crear el archivo
	int nombre_length = strlen(nombre_archivo) + 1;
	log_info(logger, "FS_THREAD -> Se va a crear el archivo %s (length %d) ", nombre_archivo, nombre_length);
	validar_conexion(socket_filesystem);
	enviar_entero(socket_filesystem, F_CREATE);
	enviar_mensaje(nombre_archivo, socket_filesystem, logger);

	int cod_op = recibir_operacion(socket_filesystem); // F_OP_OK
	if (cod_op != F_OP_OK) {
		log_error(logger, "FS_THREAD -> Error al crear archivo %s. Cod op recibido: %d", nombre_archivo, cod_op);
		return EXIT_FAILURE;
	}
	int size_buffer = recibir_entero(socket_filesystem);
	char* path = (char*)recibir_buffer(&size_buffer, socket_filesystem);
	t_archivo_abierto* archivo = crear_archivo_abierto();
	archivo->nombre = nombre_archivo;

	return archivo;
}

t_archivo_abierto* obtener_archivo_abierto(char* nombre_archivo) {
    t_archivo_abierto* archivo_encontrado = NULL;
    void buscar_archivo(t_archivo_abierto* archivo) {
        if (strcmp(archivo->nombre, nombre_archivo) == 0) {
            archivo_encontrado = archivo;
        }
    }
    list_iterate(archivos_abiertos, (void*)buscar_archivo);
    if (archivo_encontrado == NULL) {
    	log_info(logger, "FS_THREAD -> Archivo no existente, creando entrada en tabla para %s...", nombre_archivo);
    	archivo_encontrado = crear_archivo_abierto();
    	archivo_encontrado->nombre = nombre_archivo;
    	log_info(logger, "FS_THREAD -> Creado entrada de archivo para %s...", archivo_encontrado->nombre);
    }
    return archivo_encontrado;
}

char* obtener_nombre_archivo(t_pcb* pcb) {
	//log_info(logger, "PID: %d Y PC: %d y SIZE %d", pcb->pid, pcb->program_counter, list_size(pcb->instrucciones));
	t_instruccion* instruccion = (t_instruccion*)list_get(pcb->instrucciones ,pcb->program_counter - 1);
	char* nombre_archivo = (char*) list_get(instruccion->parametros,0);
	return nombre_archivo;
}

void iniciar_tablas_archivos_abiertos(void) {
	archivos_abiertos = list_create();
}

void destroy_tablas_archivos_abiertos(void) {
	list_destroy(archivos_abiertos);
}

t_archivo_abierto* crear_archivo_abierto(void) {

	t_archivo_abierto* archivo = malloc(sizeof(t_archivo_abierto));
	archivo->cant_aperturas = 0;
	archivo->puntero = 0;
	archivo->file_id = file_id++;
	archivo->mutex = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(archivo->mutex , 0);
	archivo->nombre = string_new();
	archivo->cola_bloqueados = squeue_create();

	return archivo;
}

void archivo_abierto_destroy(t_archivo_abierto* archivo) {

	//free(archivo->nombre); no eliminar, esto elimina 1 parametro de la instruccion
	squeue_destroy(archivo->cola_bloqueados);
    pthread_mutex_destroy(archivo->mutex);
    free(archivo->mutex);
    free(archivo);
}





