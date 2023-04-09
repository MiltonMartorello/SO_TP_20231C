#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <pthread.h>
#include <shared.h>

#define PATH_CONFIG "file_system.config"

void terminar_programa(int conexion, t_log* logger, t_config* config);
void* esperar_cliente_hilo (void *arg);

void conexion_con_memoria(char* ip,char* puerto,t_log* logger);
#endif /* FILESYSTEM_H_ */
