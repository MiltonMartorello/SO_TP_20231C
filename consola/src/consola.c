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

	conexion_a_kernel(ip, puerto_kernel, logger);
	socket_kernel = crear_conexion(ip, puerto_kernel);
	enviar_mensaje("Hola Kernel desde la Consola", socket_kernel, logger);
	//TODO SERIALIZAR T_PROGRAMA

	programa_destroy(programa);
	terminar_programa(socket_kernel,logger,config);
	//TODO WRAPPEAR ESTOS FREEs
	free(ip);
	free(puerto_kernel);

	EXIT_SUCCESS;
}


//TODO GENERALIZAR ESTA FUNCION EN LAS SHARED

void conexion_a_kernel(char* ip, char* puerto,t_log* logger) {
	int socket_kernel = crear_conexion(ip, puerto);
	enviar_handshake(socket_kernel,CONSOLA);

	log_info(logger,"El módulo CONSOLA se conectará con el ip %s y puerto: %s  ",ip,puerto);
	recibir_operacion(socket_kernel);
	recibir_mensaje(socket_kernel, logger);
}
