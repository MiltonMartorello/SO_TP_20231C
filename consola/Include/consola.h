#ifndef CONSOLA_H_
#define CONSOLA_H_

#include <stdio.h>
#include <stdlib.h>
#include <shared.h>
#include <commons/config.h>
#include <commons/txt.h>

#define PATH_LOG "consola.log"

void correr_consola(char*, char*);
void terminar_programa(int, t_log*, t_config*);
int conexion_a_kernel(char*, char*, t_log*);
t_buffer* serializar_programa(t_programa*);
t_buffer* serializar_instrucciones(t_list*);
#endif /* CONSOLA_H_ */
