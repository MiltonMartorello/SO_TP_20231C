#include "consola.h"

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
void terminar_programa(int conexion, t_log* logger, t_config* config)
{
	if(logger != NULL) {
		log_destroy(logger);
	}

	if(config != NULL) {
		config_destroy(config);
	}

	liberar_conexion(conexion);
}


