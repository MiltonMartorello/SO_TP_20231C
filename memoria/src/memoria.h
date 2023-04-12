#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <shared.h>
#include <commons/config.h>
#include <commons/txt.h>
#include "estructuras.h"

typedef struct {
	int socket;
	t_log *log;
} t_args_hilo_cliente;


typedef struct{
	char* puerto_escucha;
	uint32_t tam_memoria;
	uint32_t tam_segmento_0;
	uint32_t cant_segmentos;
	uint32_t retardo_memoria;
	uint32_t retardo_compactacion;
	char* algoritmo_asignacion;
}t_memoria_config;

t_memoria_config* leer_config(char *path);
void correr_servidor(t_log *logger, char *puerto) ;
int escuchar_clientes(int server_fd, t_log *logger);
int aceptar_cliente(int socket_servidor);
void procesar_cliente(void *args_hilo);
void terminar_programa(int conexion, t_log* logger, t_config* config);


#endif
