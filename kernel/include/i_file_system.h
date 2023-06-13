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

typedef struct {
    int file_id;
    char* path;
    char* nombre;
    int cant_aperturas;
    pthread_mutex_t* mutex;
} t_archivo_abierto;

extern t_colas* colas_planificacion;
extern int socket_filesystem;
extern sem_t request_file_system;
extern t_log* logger;

//Interface
t_archivo_abierto* fs_crear_archivo(char* nombre_archivo);

//Internos
void procesar_file_system(void);
t_archivo_abierto* obtener_archivo_abierto(char* nombre_archivo);
char* obtener_nombre_archivo(t_pcb* pcb);
t_instruccion* obtener_instruccion(t_pcb* pcb);
void  serializar_instruccion_fs(t_instruccion* instruccion);
// Estructuras
void iniciar_tablas_archivos_abiertos(void);
void destroy_tablas_archivos_abiertos(void);
t_archivo_abierto* crear_archivo_abierto(void);
void archivo_abierto_destroy(t_archivo_abierto* archivo);
#endif /* I_FILE_SYSTEM_H_ */
