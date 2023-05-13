#include "../include/planificador.h"

 t_colas* colas_planificacion;
 sem_t sem_grado_multiprogramacion;
 sem_t sem_nuevo_proceso;

 sem_t sem_ready_proceso;
 sem_t sem_exec_proceso;

void iniciar_colas_planificacion(void) {

	colas_planificacion = malloc(sizeof(t_colas));
	colas_planificacion->cola_block = queue_create();
	colas_planificacion->cola_exec = queue_create();
	colas_planificacion->cola_exit = queue_create();
	colas_planificacion->cola_new = queue_create();
	colas_planificacion->cola_ready = queue_create();
}

void destroy_colas_planificacion(void) {

	queue_destroy(colas_planificacion->cola_block);
	queue_destroy(colas_planificacion->cola_exec);
	queue_destroy(colas_planificacion->cola_exit);
	queue_destroy(colas_planificacion->cola_new);
	queue_destroy(colas_planificacion->cola_ready);
	free(colas_planificacion);
}

void iniciar_semaforos(int grado_multiprogramacion) {

	sem_init(&sem_grado_multiprogramacion, 0, grado_multiprogramacion);
	sem_init(&sem_nuevo_proceso, 0, 0);
	sem_init(&sem_ready_proceso, 0, 0);
	sem_init(&sem_exec_proceso, 0, 0);
}

/*
 * Quita el PCB de La cola Actual, y lo pasa a la cola de READY*/

void pasar_a_cola_ready(t_pcb* pcb, t_log* logger) {

	switch(pcb->estado_actual){
		case NEW:
			queue_pop(colas_planificacion->cola_new);
			break;
		case EXEC:
			queue_pop(colas_planificacion->cola_exec);
			break;
		case BLOCK:
			queue_pop(colas_planificacion->cola_block);
			break;
		default:
			log_error(logger, "Error, no es un estado válido");
			EXIT_FAILURE;
	}

	char* estado_anterior = estado_string(pcb->estado_actual);
	pcb->estado_actual = READY;
	queue_push(colas_planificacion->cola_ready,pcb);
	log_info(logger, "Cambio de Estado: PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>", pcb->pid, estado_anterior, estado_string(pcb->estado_actual));
	sem_post(&sem_ready_proceso);
}

void pasar_a_cola_exec(t_pcb* pcb,t_log* logger) {
	if(pcb->estado_actual != READY){
		log_error(logger, "Error, no es un estado válido");
		EXIT_FAILURE;
	}
	queue_pop(colas_planificacion->cola_ready);
	char* estado_anterior = estado_string(pcb->estado_actual);
	pcb->estado_actual = EXEC;
	queue_push(colas_planificacion->cola_exec, pcb);
	log_info(logger, "Cambio de Estado: PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>", pcb->pid, estado_anterior, estado_string(pcb->estado_actual));
	sem_post(&sem_exec_proceso);
}


char* estado_string(int cod_op) {
	switch(cod_op) {
		case 0:
			return "NEW";
			break;
		case 1:
			return "READY";
			break;
		case 2:
			return "EXEC";
			break;
		case 3:
			return "BLOCK";
			break;
		case 4:
			return "EXIT";
			break;
		default:
			printf("Error: Operación de instrucción desconocida\n");
			EXIT_FAILURE;
	}
	return NULL;
}

