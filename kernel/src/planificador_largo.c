#include "../include/planificador_largo.h"

int planificador_largo_plazo(void* args_hilo) {

	t_args_hilo_planificador* args = (t_args_hilo_planificador*) args_hilo;
	t_log* logger = args->log;
	//t_config* config = args->config;
	log_info(logger, "P_LARGO -> Inicializado Hilo Planificador de Largo Plazo");

	while(1) {
		log_info(logger, "P_LARGO -> Esperando wait de nuevo proceso");
		sem_wait(&sem_nuevo_proceso);
		log_info(logger, "P_LARGO -> Esperando wait de multiprogramación");
		sem_wait(&sem_grado_multiprogramacion);
		t_pcb* pcb = (t_pcb*)squeue_peek(colas_planificacion->cola_new);
		log_info(logger, "P_LARGO -> Se va a aceptar un nuevo proceso %d", pcb->pid);
		solicitar_nueva_tabla_de_segmento(pcb);
		pasar_a_cola_ready(pcb, logger);
	}

	return 1;
}

void solicitar_nueva_tabla_de_segmento(t_pcb* pcb) {
	validar_conexion(socket_memoria);
	log_info(logger, "P_LARGO -> Solicitando Tabla de Segmentos para PID: %d...", pcb->pid);

	//SEND
	enviar_entero(socket_memoria, MEMORY_CREATE_TABLE);
	enviar_entero(socket_memoria, pcb->pid);

	//RECV
	int cod_op = recibir_entero(socket_memoria);
	if (cod_op == MEMORY_SEGMENT_CREATED) { // 55
		int pid = recibir_entero(socket_memoria);
		if (pid != pcb->pid) {
			log_error(logger, "P_LARGO -> El Pid recibido [%d] difiere del pid del PCB [%d]", pid, pcb->pid);
			return;
		}
		log_info(logger, "P_LARGO -> Asignada Tabla de Segmentos de Memoria para PID: %d", pcb->pid);
		list_add(pcb->tabla_segmento, recibir_segmento_0());
		//loggear_tabla(pcb);
	} else {
		log_error(logger, "Error: No se pudo crear tabla de segmentos para PID [%d]: Cod %d", pcb->pid, cod_op);
		return;
	}
}

void loggear_tabla(t_pcb* pcb) {
	log_info(logger, "P_LARGO -> Tabla de PCB %d", pcb->pid);
	int cant_segmentos = list_size(pcb->tabla_segmento);
	log_info(logger, "P_LARGO -> La cantidad de segmentos es: %d", cant_segmentos);
	for (int i = 0; i < cant_segmentos; ++i) {
		t_segmento* segmento = list_get(pcb->tabla_segmento, i);
		log_info(logger, "P_LARGO -> Segmento ID: %d", segmento->segmento_id);
		log_info(logger, "P_LARGO -> Inicio: %d", segmento->inicio);
		log_info(logger, "P_LARGO -> Tamaño: %d", segmento->tam_segmento);
	}
}

t_segmento* recibir_segmento_0(void) {
	t_segmento* segmento = malloc(sizeof(t_segmento));
	segmento->segmento_id = recibir_entero(socket_memoria);
	segmento->inicio= recibir_entero(socket_memoria);
	segmento->tam_segmento = recibir_entero(socket_memoria);

	return segmento;
}

