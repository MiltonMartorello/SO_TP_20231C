/*
 ============================================================================
 Name        : memoria.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <shared.h>

int main(void) {

	t_log* logger;
	if((logger = log_create("tp0.log","TP0",1,LOG_LEVEL_INFO)) == NULL) {
		printf("no pude crear el logger \n");
		exit(1);
	}
	log_info(logger, "test de log de memoria");
	int server_fd = iniciar_servidor();
	log_info(logger, "Iniciada la conexión de servidor de memoria: %d",server_fd);
	return EXIT_SUCCESS;
}
