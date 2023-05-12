#ifndef PLANIFICADOR_LARGO_H
#define PLANIFICADOR_LARGO_H

#include <stdio.h>
#include <stdlib.h>
#include <shared.h>
#include <pthread.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/txt.h>
#include <estructuras.h>

int planificador_largo_plazo(void);
int planificador_corto_plazo(void);

#endif
