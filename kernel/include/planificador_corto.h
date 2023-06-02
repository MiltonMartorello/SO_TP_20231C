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

extern t_list* lista_recursos;

int planificador_corto_plazo(void*);
void actualizar_pcb(t_pcb* pcb, t_contexto_proceso* contexto);
void procesar_contexto(t_pcb* pcb, op_code cod_op, char* algoritmo, t_log* logger);
t_pcb* planificar(char* algoritmo, t_log* logger);

void bloqueo_io(void* pcb, void* tiempo,void* tipo_algoritmo);
void procesar_wait_recurso(void* vArgs);
void procesar_signal_recurso(void* vArgs);
void pasar_segun_algoritmo(char* algoritmo,t_pcb* proceso);
char * recibir_recurso(void);

#endif /* PLANIFICADOR_CORTO_H_ */
