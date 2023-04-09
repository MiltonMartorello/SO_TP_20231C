/*
 ============================================================================
 Name        : kernel.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <shared.h>

void conexion_con_memoria(char* ip,char* puerto,t_log* logger);

int main(void) {

	t_config* config;
	char* ip;
	char* puerto_memoria;

	t_log* logger;
	if((logger = log_create("tp0.log","TP0",1,LOG_LEVEL_INFO)) == NULL) {
		printf("no pude crear el logger \n");
		exit(1);
	}

	config = iniciar_config("./kernel.config");

	ip = config_get_string_value(config,"IP_MEMORIA");
	puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");

	conexion_con_memoria(ip,puerto_memoria,logger);

	log_info(logger, "test de log de kernel");
	int server_fd = iniciar_servidor();
	log_info(logger, "Iniciada la conexión de servidor de kernel: %d",server_fd);
	return EXIT_SUCCESS;
}

void conexion_con_memoria(char* ip,char* puerto,t_log* logger){
	int conexion_memoria = crear_conexion(ip,puerto);
	enviar_handshake(conexion_memoria,KERNEL);

	recibir_operacion(conexion_memoria);
	recibir_mensaje(conexion_memoria,logger);
}


