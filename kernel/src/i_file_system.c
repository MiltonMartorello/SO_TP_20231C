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
	    t_archivo_abierto* archivo = obtener_archivo_abierto(nombre_archivo);
	    t_instruccion* instruccion = obtener_instruccion(pcb);
	    log_info(logger, "FS_THREAD -> Recibido PID con PC en %d", pcb->program_counter);
	    log_info(logger, "FS_THREAD -> Llamando a FS por la instrucción -> %d: %s", instruccion->codigo, nombre_de_instruccion(instruccion->codigo));

	    // TODO: EN BASE A LA FUNCION DE ARCHIVO QUE SEA, ENVIAR DISTINTOS MÉTODOS. TAL VEZ CONVENGA TENER EL F_FUNCTION EN EL PCB.
	    /*if (archivo == NULL) {
	        // El archivo no está abierto, enviar mensaje al sistema de archivos para crearlo
	    	log_info(logger, "FS_THREAD -> El archivo NO existe. Se Solicitará a File System crearlo");
	    	archivo = fs_crear_archivo(nombre_archivo);
	    } else {
	    	log_info(logger, "FS_THREAD -> El archivo existe");
	    }*/
	    enviar_request_fs(instruccion, nombre_archivo);
	    int cod_respuesta = recibir_entero(socket_filesystem);
	    switch (cod_respuesta) {
			case FILE_NOT_EXISTS:
				log_info(logger, "El Archivo que se intentó abrir no existe. Enviando F_CREATE %s", nombre_archivo);
				t_archivo_abierto* archivo = fs_crear_archivo(nombre_archivo);
				log_info(logger, "Se creó el archivo %s en el path: %s", archivo->nombre, archivo->path);
				break;
			default:
				break;
		}
	}
}

void enviar_request_fs(t_instruccion* instruccion, char* nombre_archivo) {
	switch (instruccion->codigo) {
		case ci_F_OPEN:
			log_info(logger, "Enviando Request de ci_F_OPEN para el archivo %s ", nombre_archivo);
			enviar_entero(socket_filesystem, F_OPEN);
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

t_instruccion* obtener_instruccion(t_pcb* pcb) {
	return (t_instruccion*)list_get(pcb->instrucciones, pcb->program_counter -1 );
}

t_archivo_abierto* fs_crear_archivo(char* nombre_archivo) {
	 // Enviar mensaje al módulo de file system para crear el archivo
	int nombre_length = strlen(nombre_archivo) + 1;
	log_info(logger, "FS_THREAD -> Se va a crear el archivo %s de length %d ", nombre_archivo, nombre_length);
	validar_conexion(socket_filesystem);
	enviar_entero(socket_filesystem, F_CREATE);
	enviar_mensaje(nombre_archivo, socket_filesystem, logger);

	int size_buffer = recibir_operacion(socket_filesystem);
	char* path = (char*)recibir_buffer(&size_buffer, socket_filesystem);
	t_archivo_abierto* archivo = crear_archivo_abierto();
	archivo->nombre = nombre_archivo;
	archivo->path = path;

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
	archivo->file_id = file_id++;
	archivo->mutex = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(archivo->mutex , 0);
	archivo->path = string_new();
	archivo->nombre = string_new();

	return archivo;
}

void archivo_abierto_destroy(t_archivo_abierto* archivo) {

    pthread_mutex_destroy(archivo->mutex);
    free(archivo->mutex);
    free(archivo->path);
    free(archivo);
}





