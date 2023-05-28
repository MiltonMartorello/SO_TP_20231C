#include "../include/planificador_corto.h"

extern socket_cpu;

int planificador_corto_plazo(void* args_hilo) {
	t_args_hilo_planificador* args = (t_args_hilo_planificador*) args_hilo;
	t_log* logger = args->log;
	log_info(logger, "P_CORTO -> Inicializado Hilo Planificador de Corto Plazo");
	t_log* config = args->config;
    char* algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    log_info(logger, "Algoritmo: %s", algoritmo);
	while(1) {
		log_info(logger, "P_CORTO -> Esperando wait de Ready Proceso");
		sem_wait(&sem_ready_proceso);
		t_pcb* pcb = planificar(algoritmo, logger);
		ejecutar_proceso(socket_cpu, pcb, logger);
		//TODO RECIBIR UN SIGNAL EN PARTICULAR => MOTIVO
		op_code cod_op = recibir_operacion(socket_cpu);//pcb
		log_info(logger, "Recibida operación: %d", cod_op);
		t_contexto_proceso* contexto = recibir_contexto(socket_cpu, logger);
		log_info(logger, "Recibí el contexto del proceso PID: %d", contexto->pid);
		actualizar_pcb(pcb, contexto);
		log_info(logger, "Program Counter actualizado a %d",pcb->program_counter);
		procesar_contexto(pcb, cod_op, logger);
	}
	return 1;
}

void actualizar_pcb(t_pcb* pcb, t_contexto_proceso* contexto) {
	pcb->registros = contexto->registros;
	pcb->program_counter = contexto->program_counter;
	temporal_stop(pcb->tiempo_ejecucion);
}

void procesar_contexto(t_pcb* pcb, op_code cod_op, t_log* logger) {
	log_info(logger, "Procesando contexto");
	switch(cod_op) {
		case PROCESO_DESALOJADO_POR_YIELD:
			log_info(logger, "Proceso desalojado por Yield");
			pasar_a_cola_ready(pcb, logger);
			break;
		case PROCESO_FINALIZADO:
			log_info(logger, "Proceso desalojado por EXIT");
			pasar_a_cola_exit(pcb, logger, SUCCESS);
			break;
		case PROCESO_BLOQUEADO:
			log_info(logger, "Proceso desalojado por BLOQUEO");
			pasar_a_cola_blocked(pcb, logger);
			break;
		default:
			log_error(logger, "Error: La respuesta del CPU es innesperada. Cod: %d", cod_op);
			EXIT_FAILURE;
	}

}

t_pcb* planificar(char* algoritmo, t_log* logger) {
	if (string_equals_ignore_case(algoritmo, "HRRN")) {
		//t_list* lista_ordenada = list_sorted(colas_planificacion->cola_ready->elements, comparator);
	} else if (string_equals_ignore_case(algoritmo, "FIFO")){
		t_pcb* pcb = (t_pcb*)queue_peek(colas_planificacion->cola_ready);
		pasar_a_cola_exec(pcb, logger);
		log_info(logger,"PID:%d ",pcb->pid);
		return pcb;
	} else {
		log_error(logger, "Error: No existe el algoritmo: %s", algoritmo);
		EXIT_FAILURE;
	}
}
