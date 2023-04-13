#ifndef SHARED_H_
#define SHARED_H_

#include "estructuras.h"
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/string.h>
#include<commons/collections/list.h>
#include<string.h>
#include<assert.h>
#include<signal.h>

extern t_log* logger;

/*
 * SERVIDOR
 * */
void* recibir_buffer(int*, int);

int iniciar_servidor(char*);
int esperar_cliente(int,t_log*);
t_list* recibir_paquete(int);
void recibir_mensaje(int,t_log*);
void enviar_handshake(int,int);
int recibir_operacion(int);


/*
 * CLIENTE
 * */
int crear_conexion(char* ip, char* puerto);
void enviar_mensaje(char* mensaje, int socket_cliente,  t_log* logger);
t_paquete* crear_paquete(void);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
t_config* iniciar_config(char*);


/*
 * GENERAL
 * */

t_log* iniciar_logger(char*);
void terminar_programa(int, t_log*, t_config*);

#endif /* SHARED_H_ */