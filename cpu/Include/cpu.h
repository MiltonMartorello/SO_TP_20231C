#ifndef CPU_H_
#define CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <shared.h>
#include <commons/config.h>
#include <commons/txt.h>
#include "../../shared/src/estructuras.h"

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
t_pcb* recibir_pcb(int socket);
void ciclo_de_instruccion(t_pcb* pcb);
void set_valor_registro(char* nombre_registro,char* valor);
int posicion_registro(char* nombre_registro);
void devolver_cpu(t_pcb* pcb,cod_proceso estado);
void actualizar_registros_pcb(t_registro* registros);
void enviar_pcb(t_pcb* pcb,cod_proceso estado);
void armar_pcb(void);
t_list* armar_instrucciones(void);
void imprimir_registros(t_registro* registros);


#endif /* CPU_H_ */
