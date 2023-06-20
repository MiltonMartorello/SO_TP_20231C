#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <sys/mman.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <shared.h>
#include <pthread.h>
#include <fcntl.h>
#include <dirent.h>
#include <commons/config.h>
#include <commons/txt.h>
#include "estructuras.h"

/* ESTRUCTURAS */
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

typedef struct {
	void* datos;
	int inicio;
	int fin;
    struct t_bloque_indirecto* next;
} t_bloque;

typedef struct {
    int puntero_indirecto;
    struct t_bloque* bloques[64]; // es el tamaño máximo del array de punteros
} t_bloque_indirecto;

typedef struct {
    char* NOMBRE_ARCHIVO;
    int TAMANIO_ARCHIVO;
    int PUNTERO_DIRECTO;
    int PUNTERO_INDIRECTO;
} FCB;


/* CONSTANTES */

#define MAX_ARCHIVOS 50

/* VARIABLES */
t_config* config;
int socket_kernel;
int socket_memoria;
int socket_fs;
char* mapBitmap;
t_bitarray* bitmap;
t_fs_config* fs_config;
FCB* fcb;
void* bloques;
t_bitarray* bloques_bitarray;
FCB lista_archivos[100];

/* PROCEDIMIENTOS */
void cargar_config_fs(t_config* config);
void conectar_con_memoria();
void iniciar_fs();
void crear_directorio(char* path);
void iniciar_superbloque();
void iniciar_bitmap();
void iniciar_bloques();
void correr_servidor();
void recibir_request_kernel(int socket_kernel);
void leer_archivo(const char* nombreArchivo);
void escribir_archivo(const char* nombreArchivo);
void levantar_fcb(const char* nombreArchivo);
void cargar_config_fcb(t_config* config_file);
void actualizar_fcb(FCB* archivo, int nuevo_tamanio, int nuevo_directo, int nuevo_indirecto);
void persistir_fcb(FCB* archivo);
void accederBitmap(uint32_t numeroBloque, int estado);
void accederBloque(const char* nombreArchivo, uint32_t numeroBloqueArchivo, uint32_t numeroBloqueFS);
void finalizar_fs(int conexion, t_log* logger, t_config* config);
void liberarBloque(int index);
void reservarBloque(int index);
void iniciar_fcbs();

/* FUNCIONES */
int existe_fs();
int abrir_archivo(const char* nombreArchivo);
int crear_archivo(const char* nombreArchivo);
int truncar_archivo(const char* nombreArchivo);
int obtener_bloque_libre(void);
t_bloque* crear_bloque(int bloque_index_dir, int bloque_index_ind);
int cargar_archivos(FCB* lis_archivos);
#endif /* FILESYSTEM_H_ */
