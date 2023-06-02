#include "../include/planificador_corto.h"

extern socket_cpu;

int planificador_corto_plazo(void* args_hilo) {
	t_args_hilo_planificador* args = (t_args_hilo_planificador*) args_hilo;
	t_log* logger = args->log;
	log_info(logger, "P_CORTO -> Inicializado Hilo Planificador de Corto Plazo");
	t_config* config = args->config;
    char* algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    log_info(logger, "P_CORTO -> Algoritmo: %s", algoritmo);
	while(1) {
		log_info(logger, "P_CORTO -> Esperando wait de Ready Proceso");
		sem_wait(&sem_ready_proceso);
		t_pcb* pcb = planificar(algoritmo, logger);
		ejecutar_proceso(socket_cpu, pcb, logger);
		//TODO RECIBIR UN SIGNAL EN PARTICULAR => MOTIVO
		op_code cod_op = recibir_operacion(socket_cpu);//pcb
		//log_info(logger, "Recibida operación: %d", cod_op);
		t_contexto_proceso* contexto = recibir_contexto(socket_cpu, logger);
		//log_info(logger, "Recibí el contexto del proceso PID: %d", contexto->pid);
		actualizar_pcb(pcb, contexto);
		//log_info(logger, "P_CORTO -> Program Counter actualizado a %d", pcb->program_counter);
		procesar_contexto(pcb, cod_op, algoritmo, logger);
	}
	return 1;
}

void actualizar_pcb(t_pcb* pcb, t_contexto_proceso* contexto) {
	pcb->registros = contexto->registros;
	pcb->program_counter = contexto->program_counter;
	temporal_stop(pcb->tiempo_ejecucion);
}

void procesar_contexto(t_pcb* pcb, op_code cod_op, char* algoritmo, t_log* logger) {
	//log_info(logger, "Procesando contexto");
	pthread_t thread_bloqueados;
	t_args_hilo_block* args = malloc(sizeof(t_args_hilo_block));
	char* nombre;

	switch(cod_op) {
		case PROCESO_DESALOJADO_POR_YIELD:
			log_info(logger, "P_CORTO -> Proceso desalojado por Yield");
			if(string_equals_ignore_case(algoritmo, "HRRN")) {
				//log_info(logger, "Ingresando a HRRN");
				pasar_a_cola_ready_en_orden(pcb, logger, comparador_hrrn);
			} else {
				//log_info(logger, "Ingresando a FIFO");
				pasar_a_cola_ready(pcb, logger);
			}
			break;
		case PROCESO_FINALIZADO:
			log_info(logger, "P_CORTO -> Proceso desalojado por EXIT");
			pasar_a_cola_exit(pcb, logger, SUCCESS);
			break;
		case PROCESO_BLOQUEADO: //BLOQUEADO POR IO



			log_info(logger, "P_CORTO -> Proceso desalojado por BLOQUEO");
			log_info(logger, "PID: <%d> - Bloqueado por: <IO>",pcb->pid);

			int tiempo_bloqueo;
			recv(socket_cpu, &tiempo_bloqueo, sizeof(int), MSG_WAITALL);//CPU le manda el tiempo

			// (void*)pcb,
			// tiempo_bloqueo,
			// (void*) algoritmo

			args->algoritmo = algoritmo;
			args->tiempo_bloqueo = tiempo_bloqueo;
			args->pcb = pcb;
			args->logger = logger;

			pthread_create(&thread_bloqueados, NULL, (void *)bloqueo_io, (void*) args);
			pthread_detach(thread_bloqueados);

			//pasar_a_cola_blocked(pcb, logger);
			break;
		case PROCESO_DESALOJADO_POR_WAIT:

			nombre = recibir_recurso();
			log_info(logger, "P_CORTO -> Proceso desalojado para ejecutar WAIT de %s ", nombre); //TODO modificar log
			args->algoritmo = algoritmo;
			args->nombre_recurso = nombre;
			args->pcb = pcb;
			args->logger = logger;

			pthread_create(&thread_bloqueados, NULL, (void *)procesar_wait_recurso,(void*) args);
			pthread_detach(thread_bloqueados);
			break;

		case PROCESO_DESALOJADO_POR_SIGNAL:

			nombre = recibir_recurso();
			log_info(logger, "P_CORTO -> Proceso desalojado para ejecutar SIGNAL de %s ", nombre);
			args->algoritmo = algoritmo;
			args->nombre_recurso = nombre;
			args->pcb = pcb;

			pthread_create(&thread_bloqueados, NULL, (void *)procesar_signal_recurso, (void *)args);
			pthread_detach(thread_bloqueados);
			break;
		default:
			log_error(logger, "Error: La respuesta del CPU es innesperada. Cod: %d", cod_op);
			EXIT_FAILURE;
	}

}

void bloqueo_io(void* pcb, void* tiempo,void* tipo_algoritmo){

	t_pcb* proceso = (t_pcb*) pcb;
	char* algoritmo = (char*)tipo_algoritmo;

	log_info(logger,"PID: <%d> - Ejecuta IO: <%d>", proceso->pid, tiempo);
	pasar_a_cola_blocked(pcb, logger, colas_planificacion->cola_block);
	usleep((int)tiempo*1000);
	pasar_a_cola_ready(pcb, logger);
}

void procesar_wait_recurso(void* vArgs) {

	t_args_hilo_block* args = (t_args_hilo_block*) vArgs;
	char* nombre = args->nombre_recurso;
	t_log* logger = args->logger;
	log_info(logger, "Haciendo Wait de Recurso: %s",nombre);
	t_recurso* recurso = buscar_recurso(nombre, logger);
	t_pcb* pcb = args->pcb;
	char* algoritmo = args->algoritmo;


	if(recurso != NULL){
		if(recurso->instancias > 0){
			recurso->instancias--;
			log_info(logger,"PID: <%d> - Wait: <%s> - Instancias: <%d>",pcb->pid,nombre,recurso->instancias);

			pasar_segun_algoritmo(algoritmo,pcb);
		}
		else{
			pasar_a_cola_blocked(pcb, logger, recurso->cola_bloqueados);
			log_info(logger,"PID: <PID> - Bloqueado por: <%s>",nombre);
		}
	}
	else{
		pasar_a_cola_exit(pcb, logger, RESOURCE_NOT_FOUND); //TODO
	}

	free(nombre);
}

void procesar_signal_recurso(void* vArgs){

	t_args_hilo_block* args = (t_args_hilo_block*) vArgs;
	char* nombre = args->nombre_recurso;
	t_log* logger = args->logger;
	log_info(logger, "Haciendo Signal de Recurso: %s",nombre);
	t_recurso* recurso = buscar_recurso(nombre, logger);
	t_pcb* proceso = args->pcb;
	char* algoritmo = args->algoritmo;

	if(recurso != NULL){

		recurso->instancias++;
		log_info(logger,"PID: <%d> - Signal: <%s> - Instancias: <%d>",proceso->pid,nombre,recurso->instancias);

		pasar_segun_algoritmo(algoritmo, queue_pop(recurso->cola_bloqueados));
	}
	else{
		pasar_a_cola_exit(proceso, logger, RESOURCE_NOT_FOUND); //TODO
	}

	free(nombre);
}

void pasar_segun_algoritmo(char* algoritmo,t_pcb* proceso){
	if(string_equals_ignore_case(algoritmo, "HRRN")) {
		pasar_a_cola_ready_en_orden(proceso, logger, comparador_hrrn);
	} else {
		pasar_a_cola_ready(proceso, logger);
	}
}

t_pcb* planificar(char* algoritmo, t_log* logger) {
	if (string_equals_ignore_case(algoritmo, "HRRN")) {
		//t_list* lista_ordenada = list_sorted(colas_planificacion->cola_ready->elements, (bool (*)(void *, void *))comparador_hrrn);
		t_pcb* pcb = (t_pcb*)queue_peek(colas_planificacion->cola_ready);
		pasar_a_cola_exec(pcb, logger);
		//log_info(logger,"PID:%d ",pcb->pid);
		return pcb;
	} else if (string_equals_ignore_case(algoritmo, "FIFO")){
		t_pcb* pcb = (t_pcb*)queue_peek(colas_planificacion->cola_ready);
		pasar_a_cola_exec(pcb, logger);
		//log_info(logger,"PID:%d ",pcb->pid);
		return pcb;
	} else {
		log_error(logger, "Error: No existe el algoritmo: %s", algoritmo);
		EXIT_FAILURE;
	}
}

char * recibir_recurso(void)
{
	recibir_operacion(socket_cpu); //MENSAJE
	int size;
	char* nombre_recurso = (char*) recibir_buffer(&size, socket_cpu);

	return nombre_recurso;
}
