#include "../include/planificador_largo.h"

extern socket_cpu;

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

int planificador_corto_plazo(void* args_hilo) {
	t_args_hilo_planificador* args = (t_args_hilo_planificador*) args_hilo;
	t_log* logger = args->log;
	log_info(logger, "P_CORTO -> Inicializado Hilo Planificador de Corto Plazo");

	while(1) {
		log_info(logger, "P_CORTO -> Esperando wait de Ready Proceso");
		sem_wait(&sem_ready_proceso);
		t_pcb* pcb = (t_pcb*)queue_peek(colas_planificacion->cola_ready);
		log_info(logger, "P_LARGO -> Se va a Encolar un nuevo proceso %d", pcb->pid);
		log_info(logger,"Size cola %d",queue_size(colas_planificacion->cola_ready));
		pasar_a_cola_exec(pcb, logger);
		t_pcb* pcb2 = (t_pcb*)queue_peek(colas_planificacion->cola_exec);
		log_info(logger,"PID:%d ",pcb2->pid);
		ejecutar_proceso(socket_cpu, pcb, logger);
		//TODO RECIBIR UN SIGNAL EN PARTICULAR => MOTIVO
		recibir_operacion(socket_cpu);//pcb
		t_contexto_proceso* proceso = recibir_contexto(socket_cpu,logger);
	}
	return 1;
}
