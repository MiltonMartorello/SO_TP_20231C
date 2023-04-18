#include "../Include/consola.h"

void correr_consola(char* archivo_config, char* archivo_programa) {

	t_log* logger = iniciar_logger(PATH_LOG);

	t_config* config = iniciar_config(archivo_config);
	log_info(logger, "Config abierta desde %s", config->path);

	t_programa* programa = parsear_programa(archivo_programa, logger);
	printf("Size del programa fuera de parsear %d\n" ,programa->size);
	if (programa == NULL) {
		log_error(logger, "Error de parseo en archivo de pseudocódigo");
		return;
	}

	//log_info(logger, "Size del programa: %d", &programa->size);

	char* ip;
	char* puerto_kernel;
	int socket_kernel;

	ip = config_get_string_value(config,"IP_KERNEL");
	puerto_kernel = config_get_string_value(config,"PUERTO_KERNEL");
	log_info(logger, "IP: %s.",ip);
	log_info(logger, "Puerto de conexión CONSOLA-KERNEL: %s", puerto_kernel);;

	socket_kernel = conexion_a_kernel(ip, puerto_kernel, logger);
	if (socket_kernel < 0){
		log_error(logger, "Consola no pudo realizar la conexión con Kernel");
		EXIT_FAILURE;
	}

	//TODO SERIALIZAR T_PROGRAMA
	log_info(logger, "Serializando Programa");
	t_buffer* buffer = serializar_programa(programa, logger);
	log_info(logger, "Programa Serializado! con tamaño (%d) ",buffer->size);
	//enviar_paquete(t_paquete* paquete, int socket_cliente);

	programa_destroy(programa);
	terminar_programa(socket_kernel,logger,config);
	//TODO WRAPPEAR ESTOS FREEs
	free(ip);
	free(puerto_kernel);

	EXIT_SUCCESS;
}


//TODO GENERALIZAR ESTA FUNCION EN LAS SHARED

int conexion_a_kernel(char* ip, char* puerto,t_log* logger) {
	int socket_kernel = crear_conexion(ip, puerto);
	enviar_handshake(socket_kernel,CONSOLA);

	log_info(logger,"El módulo CONSOLA se conectará con el ip %s y puerto: %s  ",ip,puerto);
	recibir_operacion(socket_kernel);
	recibir_mensaje(socket_kernel, logger);
	return socket_kernel;
}

t_buffer * serializar_programa(t_programa* programa, t_log* logger){
	t_buffer* buffer;
	t_buffer* instrucciones;
	log_info(logger, "estoy dentro de la serializacion");
	instrucciones = serializar_instrucciones(programa->instrucciones, logger);

	return buffer;
}

int serializar_buffer_programa(int size_buffer, int cant_instrucciones, t_list *instrucciones) {

	t_list_iterator* iterador_instrucciones; // puntero para recorrer las instrucciones del programa
	t_list_iterator* iterador_parametros; // puntero para recorrer los parametros de una instruccion
	t_instruccion *instruccion; // La instruccion a recorrer
	char *parametro; // El parametro a recorrer
	int offset = 0; // desfasaje/corrimimento de bytes en el buffer
	int size_parametro; // Tamaño del parámetro
	int cant_parametros = 0; // Cantidad de Parámetros

	t_buffer* buffer = crear_buffer();
	// Es el Tamaño calculado + el int con las cantidad de instrucciones a enviar
	buffer->size = size_buffer + sizeof(int);
	buffer->stream = malloc(sizeof(buffer->stream));
	//MEMCPY(DESTINO, FUENTE, TAMAÑO)
	//Cantidad de instrucciones
	memcpy(buffer->stream + offset, &(cant_instrucciones), sizeof(int));
	offset += sizeof(int);
	iterador_instrucciones = list_iterator_create(instrucciones);
	while (list_iterator_has_next(iterador_instrucciones)) {
		instruccion = (t_instruccion*) list_iterator_next(
				iterador_instrucciones);
		// Código de instrucción
		memcpy(buffer->stream + offset, &(instruccion->codigo), sizeof(int));
		offset += sizeof(int);
		// Cantidad de parámetros
		cant_parametros = list_size(instruccion->parametros);
		memcpy(buffer->stream + offset, &(cant_parametros), sizeof(int));
		offset += sizeof(int);
		// Si tiene parámetros
		if (cant_parametros > 0) {
			iterador_parametros = list_iterator_create(instruccion->parametros);
			// Mientras existra otro parámetro, tal vez queda medio redundante con el if de arriba.
			while (list_iterator_has_next(iterador_parametros)) {
				parametro = (char*) list_iterator_next(iterador_parametros);
				// TODO revisar si es necesario el + 1
				size_parametro = strlen(parametro) + 1;
				// Tamaño del parámetro
				memcpy(buffer->stream + offset, &(size_parametro),
						sizeof(int));
				offset += sizeof(int);
				// Parámetro
				memcpy(buffer->stream + offset, parametro, size_parametro);
				offset += size_parametro;
			}
			list_iterator_destroy(iterador_parametros);
		}
	}
	list_iterator_destroy(iterador_instrucciones);

	if (offset != buffer->size)
		log_error(logger, "El tamaño del buffer[%d] no coincide con el offset[%d]\n", (int)buffer->size, offset);

	return offset;
}

t_buffer* serializar_instrucciones(t_list* instrucciones, t_log* logger) {
	t_buffer* buffer; //buffer a retornar
	int size_buffer = 0; // tamaño total del buffer a retornar
	int cant_instrucciones = 0; // cantidad de instrucciones
	t_list_iterator* iterador_instrucciones; // puntero para recorrer las instrucciones del programa
	t_list_iterator* iterador_parametros; // puntero para recorrer los parametros de una instruccion
	int cant_parametros = 0; // Cantidad de parámetros para validar existencia
	t_instruccion* instruccion; // La instrucción actual que se va a recorrer


	log_info(logger, "dentro de serializar instrucciones");
	iterador_instrucciones = list_iterator_create(instrucciones);
	// Mientras exista una instrucción más
	while (list_iterator_has_next(iterador_instrucciones)) {

		instruccion = (t_instruccion*)list_iterator_next(iterador_instrucciones);

		size_buffer += sizeof(int) * 2; // codigo de instruccion + cantidad de parametros
		cant_parametros  = list_size(instruccion->parametros);

		// Si tiene parámetros
		if (cant_parametros > 0) {
			iterador_parametros = list_iterator_create(instruccion->parametros);
			// Mientras exista otro parámetro más
			while(list_iterator_has_next(iterador_parametros)) {
				// Tamaño del parámetro + Tamaño del String + 1 por el endline
				// TODO revisar si es necesario el + 1
				size_buffer += sizeof(int) + strlen((char*)list_iterator_next(iterador_parametros) + 1);
			}
			// Liberamos el iterador de parámetros
			list_iterator_destroy(iterador_parametros);
		}
		// Contamos una instrucción más.
		cant_instrucciones++;
	}
	// Liberamos el iterador de instrucciones
	list_iterator_destroy(iterador_instrucciones);

	/*
	 * SERIALIZACION
	 * */
	int offset = serializar_buffer_programa(size_buffer, cant_instrucciones, instrucciones);


	return buffer;
}
