#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <commons/txt.h>
#include <pthread.h>
#include <shared.h>
#include "estructuras.h"
#include <fcntl.h>
#include <sys/mman.h>

/* -- ESTRUCTURAS -- */

typedef struct {
    uint32_t block_size;
    uint32_t block;
} Superbloque;


typedef struct {
    char nombre_archivo;
    uint32_t tamanio_archivo;
    uint32_t puntero_directo;
    uint32_t puntero_indirecto;
} FCB;

typedef struct {
    // Definir la estructura del Archivo de Bloques
} ArchivoBloques;

typedef struct
{
	char* IP_MEMORIA;
	char* PUERTO_MEMORIA;
	char* PUERTO_ESCUCHA;
    char* PATH_SUPERBLOQUE;
    char* PATH_BITMAP;
    char* PATH_BLOQUES;
    char* PATH_FCB;
    int RETARDO_ACCESO_BLOQUE;
    int BLOCK_COUNT;
    int BLOCK_SIZE;

} t_fs_config;

/* -- VARIABLES -- */
t_config* config_fs;
t_fs_config* fs_config;
int socket_fs;
int socket_kernel;
int socket_memoria;
int estado_socket_mem;
int estado_socket_kernel;
char* superBloqueMap;
Superbloque superbloque;
t_bitarray* bitmap;
ArchivoBloques archivoBloques;
FCB fcb;
char* mapBloques;
char* mapBloquesOriginal;
/* CONSTANTES */
char* mapBitmap;

#define PATH_CONFIG "file_system.config"

/* -- FUNCIONES -- */

void cargarConfigFS(t_config* config_fs);
void inicializarFS();
int existeFS();
void inicializarSuperBloque();
void finalizarFS(int conexion, t_log* logger, t_config* config);

void conectar_con_memoria();
void correr_servidor();
void abrirArchivo(const char* nombreArchivo);
void crearArchivo(const char* nombreArchivo);
void truncarArchivo(const char* nombreArchivo, uint32_t nuevoTamano);
void leerArchivo(const char* nombreArchivo, uint32_t puntero, uint32_t direccionMemoria, uint32_t tamano);
void escribirArchivo(const char* nombreArchivo, uint32_t puntero, uint32_t direccionMemoria, uint32_t tamano);
void accederBitmap(uint32_t numeroBloque, int estado);
void accederBloque(const char* nombreArchivo, uint32_t numeroBloqueArchivo, uint32_t numeroBloqueFS);
int existeArchivo(char* ruta);

#endif /* FILESYSTEM_H_ */
