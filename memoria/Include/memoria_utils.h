#ifndef MEMORIA_UTILS_H_
#define MEMORIA_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <shared.h>
#include <commons/config.h>
#include <commons/txt.h>
#include "estructuras.h"

#define SEGMENTO_0 0

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
	int pid;
	t_list* tabla;
}t_tabla_segmento;

/*
 * RECIBE CREATE_TABLE
 * RECIBE PARAMETRO PID
 * CREAR T_TABLA_SEGMENTO
 * TABLA->PID
 * TABLA->TABLA = LIST_CREATE
 *
 * BUSCAR_SEGMENTO(0)
 * LIST_ADD(TABLA->TABLA, SEGMENTO);
 * */

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

t_segmento* crear_segmento(int pid, int tam_segmento, int segmento_id);
void delete_segmento(int pid, int segmento_id);

t_tabla_segmento* crear_tabla_segmento(int pid);
void destroy_tabla_segmento(void* elemento);
t_tabla_segmento* buscar_tabla_segmentos(int pid);

t_hueco* crear_hueco(int inicio, int fin);
void actualizar_hueco(t_hueco* hueco, int nuevo_piso, int nuevo_fin);

t_memoria_config* leer_config(char *path);
void correr_servidor(t_log *logger, char *puerto);

int aceptar_cliente(int socket_servidor);

void iniciar_estructuras(void);
void destroy_estructuras(void);

void eliminar_hueco(t_hueco* hueco);
void consolidar(int inicio, int tamanio);
t_hueco* buscar_hueco(int tamanio);
t_list* filtrar_huecos_libres_por_tamanio(int tamanio);
t_hueco* buscar_hueco_por_best_fit(int tamanio);
t_hueco* buscar_hueco_por_first_fit(int tamanio);
t_hueco* buscar_hueco_por_worst_fit(int tamanio);

void loggear_huecos(t_list* huecos);
void loggear_tablas_segmentos(void);
int obtener_max_tam_segmento_para_log(t_list* tabla_segmentos);
int tamanio_hueco(t_hueco* hueco);

#endif /* MEMORIA_UTILS_H_ */
