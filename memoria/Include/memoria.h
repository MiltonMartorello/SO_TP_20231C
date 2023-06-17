#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <shared.h>
#include <commons/config.h>
#include <commons/txt.h>
#include "estructuras.h"
#include "memoria_utils.h"

extern t_espacio_usuario* espacio_usuario;
extern t_memoria_config* memoria_config;

extern t_log * logger;
extern int id;

void actualizar_hueco(t_hueco* hueco, int nuevo_piso, int nuevo_fin);

void consolidar(int inicio, int tamanio);
t_hueco* buscar_hueco(int tamanio);
t_list* filtrar_huecos_libres_por_tamanio(int tamanio);
t_hueco* buscar_hueco_por_best_fit(int tamanio);
t_hueco* buscar_hueco_por_first_fit(int tamanio);
t_hueco* buscar_hueco_por_worst_fit(int tamanio);

int escuchar_clientes(int server_fd, t_log *logger);
void procesar_cliente(void *args_hilo);
void procesar_kernel(int socket_cliente);
void enviar_tabla_segmento(int socket_kernel, t_tabla_segmento* tabla_segmento);

#endif
