#include "../include/i_file_system.h"

void procesar_file_system(void) {

	iniciar_tablas_archivos_abiertos();

	while (true) {
		sem_wait(&request_file_system);
	    // Obtener los datos necesarios del PCB para la solicitud
		t_pcb* pcb = malloc(sizeof(t_pcb)); //MOCK
	    int pid = pcb->pid;
	    char* nombre_archivo = obtener_nombre_archivo(pcb);
	    t_archivo_abierto* archivo = obtener_archivo_abierto(nombre_archivo);
	    if (archivo == NULL) {
	        // El archivo no está abierto, enviar mensaje al sistema de archivos para crearlo
	    	archivo = fs_crear_archivo(nombre_archivo);
	    }
	    t_instruccion* instruccion = obtener_instruccion(pcb);
	    serializar_instruccion_fs(instruccion);


	}
}

void   serializar_instruccion_fs(t_instruccion* instruccion) {
	//TODO
}

t_instruccion* obtener_instruccion(t_pcb* pcb) {
	return (t_instruccion*)list_get(pcb->instrucciones,pcb->program_counter);
}

t_archivo_abierto* fs_crear_archivo(char* nombre_archivo) {
	 // Enviar mensaje al módulo de file system para crear el archivo
	int cod_op = F_CREATE;
	int nombre_length = strlen(nombre_archivo) + 1;

	validar_conexion(socket_filesystem);
	enviar_entero(socket_filesystem, cod_op);

	enviar_entero(socket_filesystem, nombre_length);

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
	t_instruccion* instruccion = (t_instruccion*)list_get(pcb->instrucciones,pcb->program_counter);
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





