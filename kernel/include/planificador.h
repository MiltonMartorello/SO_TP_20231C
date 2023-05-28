#ifndef SRC_PLANIFICADOR_H_
#define SRC_PLANIFICADOR_H_

#include <estructuras.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>

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
	t_temporal* tiempo_llegada;
	t_temporal* tiempo_ejecucion;
	t_estado estado_actual;
	t_list* tabla_archivos_abiertos;
	t_list* tabla_segmento;
} t_pcb;

typedef struct {
	t_queue* cola_ready;
	t_queue* cola_new;
	t_queue* cola_exec;
	t_queue* cola_block;
	t_queue* cola_exit;
} t_colas;

void iniciar_colas_planificacion(void);
void destroy_colas_planificacion(void);
void iniciar_semaforos(int);
void destroy_semaforos(void);
t_pcb* crear_pcb(t_programa*, int);
void destroy_pcb(t_pcb*);
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
void pasar_a_cola_exec(t_pcb*, t_log*);
void pasar_a_cola_blocked(t_pcb*, t_log*);
void pasar_a_cola_exit(t_pcb*, t_log*);

char* estado_string(int);
t_registro crear_registro(void);
t_temporal* temporal_reset(t_temporal* temporal);

#endif /* SRC_PLANIFICADOR_H_ */
