#include "../include/i_console.h"

void procesar_consola(void *args_hilo) {
	t_args_hilo_cliente *args = (t_args_hilo_cliente*) args_hilo;

	int socket_consola = args->socket;
	t_log *logger = args->log;

	int cod_op = recibir_operacion(socket_consola);
	switch (cod_op) {
		case PROGRAMA:
			log_info(logger, "Recibiendo programa");
			t_buffer* buffer = recibir_buffer_programa(socket_consola, logger);
			t_programa* programa = deserializar_programa(buffer, logger);
			crear_proceso(programa, logger);
			programa_destroy(programa);
			break;
		default:
			log_error(logger, "CÓDIGO DE OPERACIÓN DESCONOCIDO.");
			break;
	}
	pthread_exit(NULL);
}

t_buffer* recibir_buffer_programa(int socket_consola, t_log* logger) {
	int size;
	t_buffer* buffer = crear_buffer();
	buffer->stream = recibir_buffer(&size, socket_consola);
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
	t_programa* programa = malloc(sizeof(t_programa));
	programa->size = 0;

	//Tamaño proceso
	memcpy(&(programa->size), buffer->stream + offset, sizeof(int));
	offset += sizeof(int);

	//Tamaño buffer instrucciones
	memcpy(&(iterador_buffer->size), buffer->stream + offset, sizeof(int));
	offset += sizeof(int);

	//instrucciones
	iterador_buffer->stream = malloc(iterador_buffer->size);
	memcpy((iterador_buffer->stream), buffer->stream + offset, iterador_buffer->size);
	offset += iterador_buffer->size;

	programa->instrucciones = deserialiar_instrucciones(iterador_buffer, logger);
	buffer_destroy(iterador_buffer);


	return programa;
}


void crear_proceso(t_programa* programa,t_log* logger){


}
