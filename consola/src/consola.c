#include "consola.h"

int main(void) {

	char* ip;
	char* puerto_kernel;
	int socket_kernel;

	t_log* logger = iniciar_logger(PATH_LOG);
	log_info(logger, "Log inicializado");

	t_config* config = iniciar_config(PATH_CONFIG);
	log_info(logger, "Config abierta desde %s", config->path);

	ip = config_get_string_value(config,"IP_KERNEL");
	puerto_kernel = config_get_string_value(config,"PUERTO_KERNEL");
	log_info(logger, "IP: %s.",ip);
	log_info(logger, "Puerto de conexión CONSOLA-KERNEL: %s", puerto_kernel);;

	socket_kernel = crear_conexion(ip, puerto_kernel);
	enviar_mensaje("Hola Kernel desde la consola", socket_kernel, logger);

	terminar_programa(socket_kernel,logger,config);

	return EXIT_SUCCESS;
}

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
