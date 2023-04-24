#ifndef I_CONSOLE_H
#define I_CONSOLE_H

#include <stdio.h>
#include <stdlib.h>
#include <shared.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/txt.h>
#include <estructuras.h>

t_buffer* recibir_buffer_programa(int, t_log*);
t_programa* deserializar_programa(t_buffer*, t_log*);
t_list* deserialiar_instrucciones(t_buffer*, t_log*);
void crear_proceso(t_programa*, t_log*);

#endif
