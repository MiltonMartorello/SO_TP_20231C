#ifndef I_FILE_SYSTEM_H_
#define I_FILE_SYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <shared.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/txt.h>
#include <estructuras.h>
#include "planificador_utils.h"


extern t_colas* colas_planificacion;
extern int socket_filesystem;
extern sem_t request_file_system;
extern sem_t f_seek_done;
extern sem_t f_close_done;
extern sem_t f_open_done;
extern t_log* logger;
extern t_list* archivos_abiertos;

//Interface
t_archivo_abierto* fs_crear_archivo(char* nombre_archivo);
void enviar_request_fs(int pid, t_instruccion* instruccion, char* nombre_archivo);

//Internos
void procesar_file_system(void);
char* obtener_nombre_archivo(t_pcb* pcb);
t_instruccion* obtener_instruccion(t_pcb* pcb);
// Estructuras
void iniciar_tablas_archivos_abiertos(void);
void destroy_tablas_archivos_abiertos(void);

#endif /* I_FILE_SYSTEM_H_ */
