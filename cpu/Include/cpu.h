#ifndef CPU_H_
#define CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <shared.h>
#include <estructuras.h>
#include <commons/config.h>
#include <commons/txt.h>

typedef struct{
    int retardo_instruccion;
    char* ip_memoria;
    char* puerto_memoria;
    char* puerto_escucha;
    int tam_max_segmento;
} t_cpu_config;

typedef struct{
    char registros_4[4][4];
    char registros_8[4][8];
    char registros_16[4][16];
} t_reg;

void cargar_config(char* path);
void conexion_a_memoria(char* ip,char* puerto,t_log* logger);
void correr_servidor(void);

void ciclo_de_instruccion(t_contexto_proceso* proceso,int socket);
void set_valor_registro(char* nombre_registro,char* valor);
int posicion_registro(char* nombre_registro);
void devolver_proceso(int,t_contexto_proceso*,int,t_log*);
void actualizar_registros_pcb(t_registro* registros);
void setear_registros_desde_proceso(t_contexto_proceso* proceso);
void loggear_registros(t_registro* registro);

char* obtener_parametro(t_list* parametros, int posicion);
t_list* armar_instrucciones(void);
void imprimir_registros(t_registro* registros);
void terminar(void);

void liberar_proceso(t_contexto_proceso* proceso);
void liberar_parametros_instruccion(void* instruccion);
#endif /* CPU_H_ */
