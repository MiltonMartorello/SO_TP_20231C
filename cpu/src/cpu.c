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

void conexion_a_memoria(char* ip,char* puerto,t_log* logger);

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

	conexion_a_memoria(ip,puerto_memoria,logger);

	//int server_fd = iniciar_servidor();
	//log_info(logger, "Iniciada la conexión de servidor de cpu: %d",server_fd);

	return EXIT_SUCCESS;

}

void conexion_a_memoria(char* ip,char* puerto,t_log* logger){
	int conexion_memoria = crear_conexion(ip,puerto);
	enviar_handshake(conexion_memoria,CPU);

	log_info(logger,"El módulo CPU se conectará con el ip %s y puerto: %s  ",ip,puerto);
	recibir_operacion(conexion_memoria);
	recibir_mensaje(conexion_memoria, logger);
}

