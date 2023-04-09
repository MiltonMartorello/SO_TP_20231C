/*
 ============================================================================
 Name        : cpu.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "cpu.h"

int main(void) {

	t_log* logger;
	t_config* config;
	char* ip;
	char* puerto_memoria;
	uint32_t socket_cpu;
	uint32_t socket_memoria;
	uint32_t socket_kernel;



	if((logger = log_create("cpu.log","CPU",1,LOG_LEVEL_INFO)) == NULL) {
		printf("no pude crear el logger \n");
		exit(1);
	}

	log_info(logger, "test de log de cpu");

	config = iniciar_config("./cpu.config");

	ip = config_get_string_value(config,"IP_MEMORIA");
	puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");
	log_info(logger,"El módulo CPU se conectará con el ip %s y puerto: %s  ",ip,puerto_memoria);

	socket_memoria = crear_conexion(ip, puerto_memoria);

	enviar_mensaje("Hola Cami", socket_memoria, logger);
	//Conexion para el kernel
	socket_cpu = iniciar_servidor(puerto_memoria);
	log_info(logger, "Iniciada la conexión de servidor de cpu: %d",socket_kernel);
	socket_kernel = esperar_cliente(socket_cpu);

	return EXIT_SUCCESS;



}


