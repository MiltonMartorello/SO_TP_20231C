#ifndef I_CONSOLE_H
#define I_CONSOLE_H

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
extern sem_t sem_exec_proceso;
extern sem_t sem_exit_proceso;
extern pthread_mutex_t mutex_cola_new;
extern pthread_mutex_t mutex_cola_exit;

extern t_kernel_config* kernel_config;

int pid_contador = 0;

t_buffer* recibir_buffer_programa(int, t_log*);
t_programa* deserializar_programa(t_buffer*, t_log*);

void crear_proceso(t_programa*, t_log*,int);
void respuesta_proceso(t_programa*, t_log*, int);
void loggear_programa(t_programa*, t_log*);
void loggear_return_kernel(int , int , t_log* );
int nuevo_pid(void);


#endif
