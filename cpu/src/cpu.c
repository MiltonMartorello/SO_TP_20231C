#include "cpu.h"

void conexion_a_memoria(char* ip,char* puerto,t_log* logger);

int main(void) {

	t_log* logger;
	t_config* config;
	char* ip;
	char* puerto_memoria;
	int socket_cpu;
	int socket_kernel;


	logger = iniciar_logger("cpu.log");

	config = iniciar_config("./cpu.config");

	ip = config_get_string_value(config,"IP_MEMORIA");
	puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");
	log_info(logger,"El módulo CPU se conectará con el ip: %s y puerto: %s  ",ip,puerto_memoria);

	conexion_a_memoria(ip,puerto_memoria,logger);

	socket_cpu = iniciar_servidor(puerto_memoria);
	log_info(logger, "Iniciada la conexión de servidor de cpu: %d",socket_cpu);

	socket_kernel = esperar_cliente(socket_cpu, logger);
	log_info(logger, "Kernel Conectado.");

	terminar_programa(socket_cpu,logger,config);

	liberar_conexion(socket_kernel);
	return EXIT_SUCCESS;
}

void conexion_a_memoria(char* ip,char* puerto,t_log* logger){
	int conexion_memoria = crear_conexion(ip,puerto);
	enviar_handshake(conexion_memoria,CPU);

	log_info(logger,"El módulo CPU se conectará con el ip %s y puerto: %s  ",ip,puerto);
	recibir_operacion(conexion_memoria);
	recibir_mensaje(conexion_memoria, logger);
}

void terminar_programa(int conexion, t_log* logger, t_config* config)
{

	if(logger != NULL){
		log_destroy(logger);
	}

	if(config != NULL){
		config_destroy(config);
	}

	liberar_conexion(conexion);
}




