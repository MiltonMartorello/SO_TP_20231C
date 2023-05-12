#include "../include/i_console.h"

void procesar_consola(void *args_hilo) {

	t_args_hilo_cliente *args = (t_args_hilo_cliente*) args_hilo;

	int socket_consola = args->socket;
	t_log* logger = args->log;

	log_info(logger, "Iniciado hilo de escucha con consola en socket %d", socket_consola);

	int estado_socket = validar_conexion(socket_consola);
	int cod_op = recibir_operacion(socket_consola);
	log_info(logger, "Recibida op code: %d", cod_op);
	switch (cod_op) {
		case PROGRAMA:
			log_info(logger, "Recibiendo programa");
			t_buffer* buffer = recibir_buffer_programa(socket_consola, logger);
			log_info(logger, "Recibido buffer de %d bytes", buffer->size);
			t_programa* programa = deserializar_programa(buffer, logger);
			loggear_programa(programa,logger);
			crear_proceso(programa, logger);
			programa_destroy(programa);
			break;
		default:
			log_error(logger, "CÓDIGO DE OPERACIÓN DESCONOCIDO. %d", cod_op);
			break;
	}
	pthread_exit(NULL);
}

t_buffer* recibir_buffer_programa(int socket_consola, t_log* logger) {
	int size;
	t_buffer* buffer = crear_buffer();
	buffer->stream = recibir_buffer(&size, socket_consola);
	buffer->size = size;
	return buffer;
}


t_list* deserialiar_instrucciones(t_buffer* buffer, t_log* logger) {
	t_list* instrucciones = list_create();
	t_instruccion* instruccion;
	int cant_instrucciones;
	int cant_parametros;
	int size_parametro;
	char* parametro;
	t_codigo_instruccion cod_instruccion;
	int offset = 0;

	log_info(logger,"Se van a deserializar las instrucciones");
	// cantidad de instrucciones
	memcpy(&cant_instrucciones, buffer->stream + offset, sizeof(int));
	offset += sizeof(int);

	// por cada instrucción que tenga el programa
	for (int i = 0; i < cant_instrucciones; i++) {

		//Código de instrucción
		memcpy(&(cod_instruccion), buffer->stream + offset, sizeof(int));
		offset += sizeof(int);

		instruccion = crear_instruccion(cod_instruccion, false);

		//Cantidad de parámetros
		memcpy(&(cant_parametros), buffer->stream + offset, sizeof(int));
		offset += sizeof(int);

		// Los casos de YIELD, EXIT no tienen parámetros y quedan con instruccion->parametros = NULL;
		if(cant_parametros > 0) {
			// por cada parámetro que tenga la instrucción
			for (int j = 0; j < cant_parametros; j++) {

				// Size del parámetro
				memcpy(&(size_parametro), buffer->stream + offset, sizeof(int));
				offset += sizeof(int);

				// Parámetro
				parametro = malloc(size_parametro);
				memcpy(parametro, buffer->stream + offset, size_parametro);
				offset += size_parametro;

				// Se inserta parámetro a la lista de parámetros
				list_add(instruccion->parametros, parametro);
			}
		}
		// Se inserta instrucción a la lista de instrucciones
		list_add(instrucciones,instruccion);
	}
	return instrucciones;
}

t_programa* deserializar_programa(t_buffer* buffer, t_log* logger){
	t_buffer* iterador_buffer = crear_buffer();
	int offset = 0;
	t_programa* programa = crear_programa(list_create());
	//t_programa* programa = malloc(sizeof(t_programa));
	//programa->size = 0;
	log_info(logger,"Se va a deserializar el programa");

	//Tamaño programa
	memcpy(&(programa->size), &(buffer->size) , sizeof(int));
	log_info(logger,"Tamaño del programa: %d", programa->size);

	//Tamaño buffer instrucciones
	memcpy(&(iterador_buffer->size), &(buffer->size), sizeof(int));
	log_info(logger,"Tamaño del buffer de instrucciones: %d", iterador_buffer->size);

	//instrucciones
	iterador_buffer->stream = malloc(iterador_buffer->size);
	memcpy((iterador_buffer->stream), buffer->stream , iterador_buffer->size);
	offset += iterador_buffer->size;

	programa->instrucciones = deserialiar_instrucciones(iterador_buffer, logger);
	buffer_destroy(iterador_buffer);

	return programa;
}


void crear_proceso(t_programa* programa, t_log* logger) {
	t_pcb* pcb = crear_pcb(programa, nuevo_pid());
	pthread_mutex_t mutex_cola_new;
	if(pthread_mutex_init(&mutex_cola_new, NULL) != 0) {
	    log_error(logger, "Error al inicializar el mutex");
	    return;
	}
	if (pthread_mutex_lock(&mutex_cola_new) != 0) {
		log_error(logger, "Mutex no pudo lockear");
	};

	queue_push(colas_planificacion->cola_new, pcb);

	if (pthread_mutex_unlock(&mutex_cola_new) != 0) {
		log_error(logger, "Mutex no pudo unlockear");
	};
	log_info(logger, "Se crea el proceso <%d> en NEW", pcb->pid);
	log_info(logger, "La cola de NEW cuenta con %d procesos", queue_size(colas_planificacion->cola_new));
	pasar_a_cola_ready(pcb, logger);
	// sem_signal(blablabla)
}

void loggear_programa(t_programa* programa,t_log* logger) {
	log_info(logger, "Se recibió un proceso de %d bytes", programa->size);
	log_info(logger, "El proceso cuenta con %d instrucciones", programa->instrucciones->elements_count);
	t_list_iterator* iterador_instrucciones;
	t_list_iterator* iterador_parametros;
	iterador_instrucciones = list_iterator_create(programa->instrucciones);
	for (int i = 0; i < programa->instrucciones->elements_count; i++) {
		t_instruccion* instruccion = (t_instruccion*) list_iterator_next(iterador_instrucciones);
		log_info(logger, "Proceso nro %d: Cód %d - %s",i+1, instruccion->codigo, nombre_de_instruccion(instruccion->codigo));
		int cant_parametros = list_size(instruccion->parametros);
		log_info(logger, "Cantidad de parámetros: %d", cant_parametros);
		iterador_parametros = list_iterator_create(instruccion->parametros);
		for (int j = 0; j < cant_parametros; j++) {
			char* parametro = (char*) list_iterator_next(iterador_parametros);
			log_info(logger, "Parámetro %d: %s", j+1,parametro);
		}
		list_iterator_destroy(iterador_parametros);
	}
	list_iterator_destroy(iterador_instrucciones);
}

int nuevo_pid(void) {
	return pid_contador++;
}
