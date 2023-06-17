#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#define PATH_CONFIG "file_system.config"

#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <commons/txt.h>
#include <pthread.h>
#include <shared.h>
#include "estructuras.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <dirent.h>

/* -- ESTRUCTURAS -- */

typedef struct {
    uint32_t block_size;
    uint32_t block_count;
} Superbloque;

typedef struct {
    char* NOMBRE_ARCHIVO; // Corregido: debe ser un array de caracteres para almacenar el nombre
    int TAMANIO_ARCHIVO;
    int PUNTERO_DIRECTO;
    int PUNTERO_INDIRECTO;
} FCB;

typedef struct {
    char* IP_MEMORIA;
    char* PUERTO_MEMORIA;
    char* PUERTO_ESCUCHA;
    char* PATH_SUPERBLOQUE;
    char* PATH_BITMAP;
    char* PATH_BLOQUES;
    char* PATH_FCB;
    int RETARDO_ACCESO_BLOQUE;
    char* PATH_FS;
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
FCB* fcb;

/* CONSTANTES */
char* mapBitmap;


/* -- FUNCIONES -- */

void cargarConfigFS(t_config* config_fs);
void inicializarFS();
int existeFS();
void finalizarFS(int conexion, t_log* logger, t_config* config);

void procesar_f_truncate(char * nombre_archivo);//TODO
void procesar_f_read(char * nombre_archivo);//TODO
void procesar_f_write(char * nombre_archivo);//TODO


void conectar_con_memoria();
void correr_servidor();
void leerArchivo(const char* nombreArchivo, uint32_t puntero, uint32_t direccionMemoria, uint32_t tamano);
void escribirArchivo(const char* nombreArchivo, uint32_t puntero, uint32_t direccionMemoria, uint32_t tamano);
void accederBitmap(uint32_t numeroBloque, int estado);
void accederBloque(const char* nombreArchivo, uint32_t numeroBloqueArchivo, uint32_t numeroBloqueFS);
int existeArchivo(char* ruta);

int abrirArchivo(const char* nombreArchivo);
int crearArchivo(const char* nombreArchivo);
int truncarArchivo(const char* nombreArchivo);
void levantarFCB(const char* nombreArchivo);
void persistirFCB(FCB* archivo, const char* nombre_archivo);
void actualizarFCB(FCB* archivo, int nuevo_tamanio, int nuevo_directo, int nuevo_indirecto);
int obtenerPosIniDeNBloquesLibresEnBitmap(int cantidadBloques);
void inicializarSuperBloque();
void inicializarBloques();
void inicializarBitmap();
void crearDirectorio(char* path);

void recibir_request_kernel(int socket_kernel);

#endif /* FILESYSTEM_H_ */
