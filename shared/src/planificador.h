
#ifndef SRC_PLANIFICADOR_H_
#define SRC_PLANIFICADOR_H_
#include "estructuras.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>

t_colas* colas_planificacion;

void iniciar_colas_planificacion(void);
void destroy_colas_planificacion(void);
char* estado_string(int);
void pasar_a_cola_ready(t_pcb*, t_log*);

#endif /* SRC_PLANIFICADOR_H_ */
