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
	enviar_mensaje("Hola Kernel desde la Consola", socket_kernel, logger);
	//TODO SERIALIZAR T_PROGRAMA
	t_buffer* buffer = serializar_programa(programa);
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

t_buffer * serializar_programa(t_programa* programa){
	t_buffer* buffer;
	t_buffer* instrucciones;

	instrucciones = serializar_instrucciones(programa->instrucciones);

	return buffer;
}

/*
 *	Tipo de datos		Tamaño			Descripcion
 *	uint32_t 			4 bytes			Cantidad de instrucciones
 *	uint32_t			4 bytes			Codigo de instruccion
 *	uint32_t			4 bytes			Cantidad de parametros
 *	uint32_t			4 bytes			Longitud del parametro - incluye al centinela de fin de cadena "\n"
 *	char				variable		Valor del parametro - 1 byte por caracter + centinela
 */

t_buffer* serializar_instrucciones(t_list* instrucciones) {
	t_buffer* buffer; //buffer a retornar
	int size_buffer = 0; // tamaño total del buffer a retornar
	int cant_instrucciones = 0; // cantidad de instrucciones
	t_list_iterator* iterador_instrucciones; // puntero para recorrer las instrucciones del programa
	t_list_iterator* iterador_parametros; // puntero para recorrer los parametros de una instruccion
	int size_parametros = 0; // Cantidad de parámetros para validar existencia
	t_instruccion* instruccion; // La instrucción actual que se va a recorrer

	iterador_instrucciones = list_iterator_create(instrucciones);

	/*
	 *  */
	// Mientras exista una instrucción más
	while (list_iterator_has_next(iterador_instrucciones)) {

		instruccion = (t_instruccion*)list_iterator_next(iterador_instrucciones);

		size_buffer += sizeof(int) * 2; // codigo de instruccion
		size_parametros  = list_size(instruccion->parametros);

		// Si tiene parámetros
		if (size_parametros > 0) {
			iterador_parametros = list_iterator_create(instruccion->parametros);
			// Mientras exista otro parámetro más
			while(list_iterator_has_next(iterador_parametros)) {
				// Tamaño del parámetro + Tamaño del String + 1 por el endline
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

	buffer = crear_buffer();

	return buffer;
}
