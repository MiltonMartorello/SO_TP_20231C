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
#include "planificador.h"

extern t_colas* colas_planificacion;
extern sem_t sem_grado_multiprogramacion;
extern sem_t sem_nuevo_proceso;

int planificador_largo_plazo(void*);
int planificador_corto_plazo(void*);

#endif
