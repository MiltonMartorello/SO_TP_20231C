#ifndef I_CONSOLE_H
#define I_CONSOLE_H

#include <stdio.h>
#include <stdlib.h>
#include <shared.h>
#include <pthread.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/txt.h>
#include <estructuras.h>
#include <planificador.h>

int pid_contador = 0;

t_buffer* recibir_buffer_programa(int, t_log*);
t_programa* deserializar_programa(t_buffer*, t_log*);
t_list* deserialiar_instrucciones(t_buffer*, t_log*);
void crear_proceso(t_programa*, t_log*);
void loggear_programa(t_programa*, t_log*);
int nuevo_pid(void);


#endif