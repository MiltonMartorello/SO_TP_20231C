#include "../include/i_console.h"


void procesar_consola(void *args_hilo) {

	t_args_hilo_cliente *args = (t_args_hilo_cliente*) args_hilo;

	int socket_consola = args->socket;
	t_log* logger = args->log;
	int socket_cpu = args->socket_cpu;

	log_info(logger, "Iniciado nuevo Hilo de escucha con consola en socket %d", socket_consola);

	int estado_socket = validar_conexion(socket_consola);
	int cod_op = recibir_operacion(socket_consola);
	switch (cod_op) {
		case PROGRAMA:
			t_buffer* buffer = recibir_buffer_programa(socket_consola, logger);
			t_programa* programa = deserializar_programa(buffer, logger);
//			loggear_programa(programa,logger);
			crear_proceso(programa, logger,socket_cpu);
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




t_programa* deserializar_programa(t_buffer* buffer, t_log* logger){
	t_buffer* iterador_buffer = crear_buffer();
	int offset = 0;
	t_programa* programa = crear_programa(list_create());
	//t_programa* programa = malloc(sizeof(t_programa));
	//programa->size = 0;
//	log_info(logger,"Se va a deserializar el programa");

	//Tamaño programa
	memcpy(&(programa->size), &(buffer->size) , sizeof(int));
//	log_info(logger,"Tamaño del programa: %d", programa->size);

	//Tamaño buffer instrucciones
	memcpy(&(iterador_buffer->size), &(buffer->size), sizeof(int));
//	log_info(logger,"Tamaño del buffer de instrucciones: %d", iterador_buffer->size);

	//instrucciones
	iterador_buffer->stream = malloc(iterador_buffer->size);
	memcpy((iterador_buffer->stream), buffer->stream , iterador_buffer->size);
	offset += iterador_buffer->size;

	programa->instrucciones = deserializar_instrucciones(iterador_buffer, logger);
	buffer_destroy(iterador_buffer);

	return programa;
}


void crear_proceso(t_programa* programa, t_log* logger,int socket_cpu) {
	t_pcb* pcb = crear_pcb(programa, nuevo_pid());
	log_info(logger,"Se creo un pcb con %d instrucciones",list_size(pcb->instrucciones));
	pthread_mutex_t mutex_cola_new;
	if(pthread_mutex_init(&mutex_cola_new, NULL) != 0) {
	    log_error(logger, "Error al inicializar el mutex");
	    return;
	}
	if (pthread_mutex_lock(&mutex_cola_new) != 0) {
		log_error(logger, "Mutex no pudo lockear");
	};

	queue_push(colas_planificacion->cola_new, pcb);
	//ejecutar_proceso(socket_cpu,pcb,logger);

	if (pthread_mutex_unlock(&mutex_cola_new) != 0) {
		log_error(logger, "Mutex no pudo unlockear");
	};
	log_info(logger, "Se crea el proceso <%d> en NEW", pcb->pid);
	sem_post(&sem_nuevo_proceso);
	//log_info(logger, "La cola de NEW cuenta con %d procesos", queue_size(colas_planificacion->cola_new));
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
