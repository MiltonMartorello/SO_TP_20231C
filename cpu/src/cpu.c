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


	if((logger = log_create("tp0.log","TP0",1,LOG_LEVEL_INFO)) == NULL) {
		printf("no pude crear el logger \n");
		exit(1);
	}

	log_info(logger, "test de log de cpu");

	config = iniciar_config("./cpu.config");

	ip = config_get_string_value(config,"IP_MEMORIA");
	puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");


	log_info(logger,"El módulo CPU se conectará con el ip %s y puerto: %s  ",ip,puerto_memoria);
	int server_fd = iniciar_servidor();
	log_info(logger, "Iniciada la conexión de servidor de cpu: %d",server_fd);
	return EXIT_SUCCESS;



}


