#include "../Include/consola.h"

void correr_consola(char* archivo_config, char* archivo_programa) {

	t_log* logger = iniciar_logger(PATH_LOG);

	t_config* config = iniciar_config(archivo_config);
	log_info(logger, "Config abierta desde %s", config->path);

	t_programa* programa = parsear_programa(archivo_programa, logger);
	if (programa == NULL) {
		log_error(logger, "Error de parseo en archivo de pseudocódigo");
		return;
	}

	//log_info(logger, "Size del programa: %d", &programa->size);

	char* ip;
	char* puerto_kernel;
	int socket_kernel;
	int response_code_kernel;
	ip = config_get_string_value(config,"IP_KERNEL");
	puerto_kernel = config_get_string_value(config,"PUERTO_KERNEL");
	log_info(logger, "IP: %s.",ip);
	log_info(logger, "Puerto de conexión CONSOLA-KERNEL: %s", puerto_kernel);;

	socket_kernel = conexion_a_kernel(ip, puerto_kernel, logger);
	if (socket_kernel < 0){
		log_error(logger, "Consola no pudo realizar la conexión con Kernel");
		EXIT_FAILURE;
	}

	log_info(logger, "Serializando Programa");
	t_buffer* buffer = serializar_programa(programa, logger);
	programa_destroy(programa);

	response_code_kernel = enviar_programa(buffer, socket_kernel, PROGRAMA, logger);
	//buffer_destroy(buffer);

	// Si el send envió algo
	if (response_code_kernel > 0) {
		t_list* respuesta = list_create();
		respuesta = recibir_paquete(socket_kernel, logger);

		if (list_is_empty(respuesta)) {
			log_error(logger, "La lista de respuesta, esta vacía");
			EXIT_FAILURE;
		}
		else if ((op_code)list_get(respuesta,0) != PROGRAMA_FINALIZADO) {
			log_error(logger, "Error al finalizar programa. Se esperaba código %d, y se recibió %d", PROGRAMA_FINALIZADO, (op_code)list_get(respuesta,0));
		}
		else {
			log_info(logger, "Programa ha finalizado correctamente");
		}
	}
	t_queue* cola = queue_create();
	log_info(logger, "Finalizando programa...");
	terminar_programa(socket_kernel,logger,config);

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

int enviar_programa(t_buffer* buffer, int socket_kernel, int codigo, t_log* logger){
	int response = 0;
	// llega 2 ints además del size del buffer
	// Esto esta asi por la necesidad de enviar instrucciones de la consola a kernel. Si no aplica en general, particularizamos en varias funciones.
	// 1x4 -> Código de operación
	// 1x4 -> Tamaño total
	t_paquete * paquete = crear_paquete(codigo);
	log_info(logger, "hasta este momento el buffer->size tiene %d bytes", buffer->size);
	int bytes_a_enviar = buffer->size + sizeof(int) * 2;
	log_info(logger, "Bytes totales a enviar: %d. a traves del socket %d", bytes_a_enviar, socket_kernel);
	paquete->buffer = buffer;
	void* a_enviar = serializar_paquete(paquete, bytes_a_enviar);
/*	int cod_log;
	memcpy(&cod_log, a_enviar, sizeof(int));
	log_info(logger, "Enviando %d bytes del cod_op %d a traves del socket %d", sizeof(int), cod_log, socket_kernel);
	enviar_handshake(socket_kernel, PROGRAMA);*/
	log_info(logger, "Enviando %d bytes del programa a traves del socket %d", bytes_a_enviar, socket_kernel);
	response = send(socket_kernel, a_enviar, bytes_a_enviar, 0);
	log_info(logger, "Enviados %d bytes al kernel", response);
	if (response <= 0) {
		log_error(logger,"Error al enviar el programa al kernel");
	} else if (response < bytes_a_enviar){
		log_error(logger,"Transferencia incompleta. Estimado: %d. Enviado: %d", bytes_a_enviar, response);
	}

	free(a_enviar);
	eliminar_paquete(paquete);

	return response;
}

t_buffer * serializar_programa(t_programa* programa, t_log* logger){
	t_buffer* buffer;
	t_buffer* instrucciones;
	int offset = 0;
	instrucciones = serializar_instrucciones(programa->instrucciones, logger); // 336 bytes = instrucciones + cant_instrucciones
	buffer = crear_buffer();
	// Tamaño total - Lista de instrucciones
	buffer->size = instrucciones->size;
	buffer->stream = malloc(buffer->size);
	log_info(logger,"Tengo un buffer de instrucciones con tamaño %d", instrucciones->size);
	log_info(logger,"Tengo un buffer total con tamaño %d", buffer->size);
/*
	// Tamaño total del buffer
	memcpy(buffer->stream + offset, &(buffer->size), sizeof(int));
	offset += sizeof(int);

	// Lista de instrucciones
	memcpy(buffer->stream + offset , instrucciones->stream ,instrucciones->size);
	offset += instrucciones->size;

	// Liberamos las instruciones serializadas
	buffer_destroy(instrucciones);
	if (offset != instrucciones->size)
			log_error(logger, "El tamaño del buffer[%d] no coincide con el offset[%d]\n", (int)instrucciones->size, offset);
*/

	return instrucciones;
}

int serializar_buffer_programa(int size_buffer, int cant_instrucciones, t_list* instrucciones) {

	t_list_iterator* iterador_instrucciones; // puntero para recorrer las instrucciones del programa
	t_list_iterator* iterador_parametros; // puntero para recorrer los parametros de una instruccion
	t_instruccion* instruccion; // La instruccion a recorrer
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
	log_info(logger, "Se encontró una lista de: %d",list_size(instrucciones));


	while (list_iterator_has_next(iterador_instrucciones)) {
		log_info(logger, "Entranado al index: %d",iterador_instrucciones->index);
		instruccion = (t_instruccion*)list_iterator_next(iterador_instrucciones);
		// Código de instrucción
		log_info(logger, "El código OP a alocar es: %d", instruccion->codigo);
		log_info(logger, "Este código de instruccion pesa %lu", sizeof(t_codigo_instruccion));
		memcpy(buffer->stream + offset, &(instruccion->codigo), sizeof(t_codigo_instruccion));
		offset += sizeof(t_codigo_instruccion);
		// Cantidad de parámetros
		cant_parametros = list_size(instruccion->parametros);
		log_info(logger, "voy a alocar %d bytes", cant_parametros);
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
	int antes = 0;

	iterador_instrucciones = list_iterator_create(instrucciones);
	// Mientras exista una instrucción más
	while (list_iterator_has_next(iterador_instrucciones)) {

		instruccion = (t_instruccion*)list_iterator_next(iterador_instrucciones);
		//log_info(logger, "El código OP a alocar es: %d", instruccion->codigo);

		size_buffer += sizeof(int) + sizeof(int); // codigo de instruccion + cantidad de parametros
		cant_parametros  = list_size(instruccion->parametros);

		// Si tiene parámetros
		if (cant_parametros > 0) {
			iterador_parametros = list_iterator_create(instruccion->parametros);
			// Mientras exista otro parámetro más
			while(list_iterator_has_next(iterador_parametros)) {
				// Tamaño del parámetro + Tamaño del String + 1 por el endline
				// TODO revisar si es necesario el + 1
				antes = size_buffer;
				size_buffer += sizeof(int) + strlen((char*)list_iterator_next(iterador_parametros)) + 1;
				log_info(logger, "Este parámetro de instruccion pesa %d", size_buffer - antes - 5);
			}
			// Liberamos el iterador de parámetros
			list_iterator_destroy(iterador_parametros);
		}
		// Contamos una instrucción más.
		cant_instrucciones++;
	}
	// Liberamos el iterador de instrucciones
	list_iterator_destroy(iterador_instrucciones);
	log_info(logger, "Análisis de serializado: %d Instrucciones, con un tamaño de buffer total de: %d", cant_instrucciones, size_buffer);
	/*
	 * SERIALIZACION
	 * */
	//t_list_iterator* iterador_instrucciones; // puntero para recorrer las instrucciones del programa
	//t_list_iterator* iterador_parametros; // puntero para recorrer los parametros de una instruccion
	//t_instruccion* instruccion; // La instruccion a recorrer
	char *parametro; // El parametro a recorrer
	int offset = 0; // desfasaje/corrimimento de bytes en el buffer
	int size_parametro; // Tamaño del parámetro
	//int cant_parametros = 0; // Cantidad de Parámetros

	buffer = crear_buffer();
	// Es el Tamaño calculado + el int con las cantidad de instrucciones a enviar
	buffer->size = size_buffer + sizeof(int);
	buffer->stream = malloc(buffer->size);

	//MEMCPY(DESTINO, FUENTE, TAMAÑO)
	//Cantidad de instrucciones
	memcpy(buffer->stream + offset, &(cant_instrucciones), sizeof(int));
	offset += sizeof(int);
	iterador_instrucciones = list_iterator_create(instrucciones);
	//log_info(logger, "Se encontró una lista de: %d",list_size(instrucciones));


	while (list_iterator_has_next(iterador_instrucciones)) {
		instruccion = (t_instruccion*)list_iterator_next(iterador_instrucciones);
		log_info(logger, "Entranado al index: %d",iterador_instrucciones->index);
		// Código de instrucción
		log_info(logger, "El código OP a alocar es: %d", instruccion->codigo);
		memcpy(buffer->stream + offset, &(instruccion->codigo), sizeof(int));
		offset += sizeof(int);
		// Cantidad de parámetros
		cant_parametros = list_size(instruccion->parametros);
		log_info(logger, "voy a alocar %d parametros", cant_parametros);
		memcpy(buffer->stream + offset, &cant_parametros, sizeof(int));
		offset += sizeof(int);
		// Si tiene parámetros
		if (cant_parametros > 0) {
			iterador_parametros = list_iterator_create(instruccion->parametros);
			// Mientras existra otro parámetro, tal vez queda medio redundante con el if de arriba.
			while (list_iterator_has_next(iterador_parametros)) {
				parametro = (char*) list_iterator_next(iterador_parametros);
				// TODO revisar si es necesario el + 1
				log_info(logger, "el parámetro %s, mide %d", parametro, (int)(strlen(parametro) + 1));
				size_parametro = strlen(parametro) + 1;
				// Tamaño del parámetro
				memcpy(buffer->stream + offset, &(size_parametro), sizeof(int));
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
	else{
		log_info(logger, "El tamaño del buffer[%d] coincide con el offset[%d]\n", (int)buffer->size, offset);
	}
	//int offset = serializar_buffer_programa(size_buffer, cant_instrucciones, instrucciones);


	return buffer;
}
