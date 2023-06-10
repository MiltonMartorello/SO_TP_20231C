#ifndef PLANIFICADOR_CORTO_H_
#define PLANIFICADOR_CORTO_H_

#include <stdio.h>
#include <stdlib.h>
#include <shared.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/txt.h>
#include <estructuras.h>
#include "planificador_utils.h"


extern t_colas* colas_planificacion;
extern sem_t sem_grado_multiprogramacion;
extern sem_t sem_nuevo_proceso;
extern sem_t sem_ready_proceso;
extern sem_t sem_exec_proceso;

extern sem_t cpu_liberada;
extern sem_t proceso_enviado;

extern pthread_mutex_t mutex_cola_ready;

extern t_list* lista_recursos;

extern t_kernel_config* kernel_config;


int planificador_corto_plazo(void*);
void actualizar_pcb(t_pcb* pcb, t_contexto_proceso* contexto);
void procesar_contexto(t_pcb* pcb, op_code cod_op, char* algoritmo, t_log* logger);
t_pcb* planificar(char* algoritmo, t_log* logger);
void bloqueo_io(void* vArgs);
void procesar_wait_recurso(char* nombre,t_pcb* pcb,char* algoritmo,t_log* logger);
void procesar_signal_recurso(char* nombre,t_pcb* pcb,char* algoritmo,t_log* logger);
void pasar_a_ready_segun_algoritmo(char* algoritmo,t_pcb* proceso,t_log* logger);
char * recibir_recurso(void);
t_pcb* proximo_proceso_hrrn(t_log* logger);
void loggear_registros(t_registro registro, t_log* logger);
bool comparador_hrrn(void* pcb1, void* pcb2);
void manejar_respuesta_cpu(void* args_hilo);
#endif /* PLANIFICADOR_CORTO_H_ */
