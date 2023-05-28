#include "../include/planificador_largo.h"

int planificador_largo_plazo(void* args_hilo) {

	t_args_hilo_planificador* args = (t_args_hilo_planificador*) args_hilo;
	t_log* logger = args->log;
	t_config* config = args->config;
	log_info(logger, "P_LARGO -> Inicializado Hilo Planificador de Largo Plazo");

	while(1) {
		log_info(logger, "P_LARGO -> Esperando wait de nuevo proceso");
		sem_wait(&sem_nuevo_proceso);
		log_info(logger, "P_LARGO -> Esperando wait de multiprogramación");
		sem_wait(&sem_grado_multiprogramacion);
		t_pcb* pcb = (t_pcb*)queue_peek(colas_planificacion->cola_new);
		log_info(logger, "P_LARGO -> Se va a Encolar un nuevo proceso %d", pcb->pid);
		log_info(logger,"Se paso un pcb con %d instrucciones -- planificador",list_size(pcb->instrucciones));
		pasar_a_cola_ready(pcb, logger);
		log_info(logger,"Se paso un pcb con %d instrucciones -- planificador",list_size(pcb->instrucciones));
	}

	return 1;
}

