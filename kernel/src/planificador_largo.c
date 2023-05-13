#include "../include/planificador_largo.h"

int planificador_largo_plazo(void* args_hilo) {

	t_args_hilo_planificador* args = (t_args_hilo_planificador*) args_hilo;
	t_log* logger = args->log;
	t_config* config = args->config;
	log_info(logger, "Inicializado Hilo Planificador de Largo Plazo");

	while(1) {
		log_info(logger, "Esperando wait de nuevo proceso");
		sem_wait(&sem_nuevo_proceso);
		log_info(logger, "Esperando wait de multiprogramación");
		sem_wait(&sem_grado_multiprogramacion);
		t_pcb* pcb = (t_pcb*)queue_peek(colas_planificacion->cola_new);
		log_info(logger, "Se va a Encolar un nuevo proceso %d", pcb->pid);
		pasar_a_cola_ready(pcb, logger);
	}

	return 1;
}

int planificador_corto_plazo(void* args_hilo) {
	t_args_hilo_planificador* args = (t_args_hilo_planificador*) args_hilo;
	t_log* logger = args->log;


	log_info(logger, "Inicializado Hilo Planificador de Corto Plazo");
	return 1;
}
