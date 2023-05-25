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
t_pcb* crear_pcb(t_programa*, int);
void destroy_pcb(t_pcb*);
void ejecutar_proceso(int, t_pcb*, t_log*);
void pasar_a_cola_ready(t_pcb*, t_log*);
void pasar_a_cola_exec(t_pcb*, t_log*);

char* estado_string(int);
t_registro crear_registro(void);

#endif /* SRC_PLANIFICADOR_H_ */