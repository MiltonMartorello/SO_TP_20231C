#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <shared.h>
#include <commons/config.h>
#include <commons/txt.h>
#include "estructuras.h"

typedef struct {
	char* puerto_escucha;
	uint32_t tam_memoria;
	uint32_t tam_segmento_0;
	uint32_t cant_segmentos;
	uint32_t retardo_memoria;
	uint32_t retardo_compactacion;
	char* algoritmo_asignacion;
}t_memoria_config;

typedef struct {
	int segmento_id;
	int inicio;
	int tam_segmento;
	void* valor;
}t_segmento;

typedef struct {
	int pid;
	t_list* t_segmento_tabla;
}t_tabla_segmento;
/*
 * PCB
 * PID -> 1
 * TABLA_ARCHIVOS BLA
 * TABLA_DE_SEGMENTO {
	 * SEGMENTO_0 {
	 * 		SEGMENTO_ID
	 * 		INICIO
	 *
	 * }, <-- EXISTE SIEMPRE ESC OMPARTIDO
	 * SEGMENTO_1, <-- MEDIANTE CREATE_SEGMENT
 * }
 * */

typedef struct {
	int inicio;
	int fin;
}t_hueco;

typedef struct {
	void* espacio_usuario;
	t_list* segmentos_activos;
	t_list* huecos_libres;
	//t_segmento segmento_0;
}t_espacio_usuario;

t_log * logger;
int id = 0;

t_segmento* crear_segmento(int tam_segmento);
void destroy_segmento(int id);
t_hueco* crear_hueco(int inicio, int fin);
void actualizar_hueco(t_hueco* hueco, int nuevo_piso);

t_memoria_config* leer_config(char *path);
void correr_servidor(t_log *logger, char *puerto);
int escuchar_clientes(int server_fd, t_log *logger);
int aceptar_cliente(int socket_servidor);
void procesar_cliente(void *args_hilo);
void iniciar_estructuras(void);
void destroy_estructuras(void);


#endif
