#ifndef SRC_PLANIFICADOR_UTILS_H_
#define SRC_PLANIFICADOR_UTILS_H_

#include <estructuras.h>
#include <errno.h>
#include <commons/string.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stdio.h>
#include <pthread.h>

typedef enum{
	NEW,
	READY,
	EXEC,
	BLOCK,
	EXIT
} t_estado;

// PCB
typedef struct {
	int pid;
	t_list* instrucciones;
	int program_counter;
	t_registro registros;
	int estimado_rafaga;
	int nuevo_estimado;
	t_temporal* tiempo_llegada;
	t_temporal* tiempo_ejecucion;
	t_estado estado_actual;
	t_list* tabla_archivos_abiertos;
	t_list* tabla_segmento;
	return_code motivo;
} t_pcb;

typedef struct {
	t_queue* cola_ready;
	t_queue* cola_new;
	t_queue* cola_exec;
	t_queue* cola_block;
	t_queue* cola_exit;
} t_colas;

typedef struct {
    char* pids;
} ConcatenacionPIDs;

typedef struct {
	char* nombre;
	int instancias;
	t_queue* cola_bloqueados;
} t_recurso;

typedef struct {
	t_pcb* pcb;
	int tiempo_bloqueo;
	char* algoritmo;
	char* nombre_recurso;
	t_log* logger;
} t_args_hilo_block;

/* -- ESTRUCTURAS -- */
typedef struct
{
	char* IP_MEMORIA;
	char* PUERTO_MEMORIA;
	char* IP_FILESYSTEM;
	char* PUERTO_FILESYSTEM;
	char* IP_CPU;
	char* PUERTO_CPU;
	char* PUERTO_ESCUCHA;
    char* ALGORITMO_PLANIFICACION;
    int ESTIMACION_INICIAL;
    int HRRN_ALFA;
    int GRADO_MAX_MULTIPROGRAMACION;
    char** RECURSOS;
    char** INSTANCIAS_RECURSOS;

} t_kernel_config;

void iniciar_colas_planificacion(void);
void destroy_colas_planificacion(void);
void iniciar_semaforos(int);
void destroy_semaforos(void);
t_pcb* crear_pcb(t_programa*, int);
void destroy_pcb(t_pcb*);
/*
 * Crea una estructura t_contexto en base a un pcb y lo envía al cpu
 * */
void ejecutar_proceso(int, t_pcb*, t_log*);
/*
 * Quita el PCB de La cola Actual, y lo pasa a la cola de READY
 * Activa Timer de espera
 * */
void pasar_a_cola_ready(t_pcb*, t_log*);
/*
 * Quita el PCB de La cola READY, y lo pasa a la cola de EXEC
 * Activa Timer de ejecución
 * */
void pasar_a_cola_ready_en_orden(t_pcb* pcb_nuevo, t_log* logger, int(*comparador)(t_pcb*, t_pcb*, t_log*));

void pasar_a_cola_exec(t_pcb*, t_log*);
void pasar_a_cola_blocked(t_pcb*, t_log*,t_queue*);
void pasar_a_cola_exit(t_pcb*, t_log*, return_code);

char* concatenar_pids(t_list*);
void loggear_cola_ready(t_log* logger, char* algoritmo);
char* estado_string(int);
t_registro crear_registro(void);
t_temporal* temporal_reset(t_temporal* temporal);

void iniciar_recursos(char** recursos, char** instancias);
int buscar_recurso(char* nombre, t_log* logger);
double calcular_estimado_proxima_rafaga (t_pcb* pcb, t_log* logger);
#endif /* SRC_PLANIFICADOR_UTILS_H_ */
