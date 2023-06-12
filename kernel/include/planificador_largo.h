#ifndef PLANIFICADOR_LARGO_H
#define PLANIFICADOR_LARGO_H

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

// SOCKETS
extern int socket_file_system;
extern t_colas* colas_planificacion;
extern sem_t sem_grado_multiprogramacion;
extern sem_t sem_nuevo_proceso;
extern sem_t sem_ready_proceso;
extern sem_t sem_exec_proceso;
extern t_kernel_config* kernel_config;


int planificador_largo_plazo(void*);

#endif
