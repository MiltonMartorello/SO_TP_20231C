#include "../include/planificador_corto.h"

extern int socket_cpu;
t_log* logger;

int planificador_corto_plazo(void* args_hilo) {
	t_args_hilo_planificador* args = (t_args_hilo_planificador*) args_hilo;
	logger = args->log;
	log_info(logger, "P_CORTO -> Inicializado Hilo Planificador de Corto Plazo");
	t_config* config = args->config;
    char* algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    log_info(logger, "P_CORTO -> Algoritmo: %s", algoritmo);
	while(1) {
		//log_info(logger, "P_CORTO -> Esperando Nuevo Proceso...");
		sem_wait(&sem_ready_proceso);
		t_pcb* pcb = planificar(algoritmo, logger);
		//loggear_registros(pcb->registros, logger);
		ejecutar_proceso(socket_cpu, pcb, logger);
		//TODO RECIBIR UN SIGNAL EN PARTICULAR => MOTIVO
		op_code cod_op = recibir_operacion(socket_cpu);//pcb
		//log_info(logger, "Recibida operación: %d", cod_op);
		t_contexto_proceso* contexto = recibir_contexto(socket_cpu, logger);
		//loggear_registros(contexto->registros, logger);
		//log_info(logger, "Recibí el contexto del proceso PID: %d", contexto->pid);
		actualizar_pcb(pcb, contexto);
		//log_info(logger, "P_CORTO -> Program Counter actualizado a %d", pcb->program_counter);
		procesar_contexto(pcb, cod_op, algoritmo, logger);
	}
	return 1;
}

void loggear_registros(t_registro registro, t_log* logger) {
    log_info(logger, "Valores del registro:");
    log_info(logger, "AX: %.*s", 4, registro.AX);
    log_info(logger, "BX: %.*s", 4, registro.BX);
    log_info(logger, "CX: %.*s", 4, registro.CX);
    log_info(logger, "DX: %.*s", 4, registro.DX);
    log_info(logger, "EAX: %.*s", 8, registro.EAX);
    log_info(logger, "EBX: %.*s", 8, registro.EBX);
    log_info(logger, "ECX: %.*s", 8, registro.ECX);
    log_info(logger, "EDX: %.*s", 8, registro.EDX);
    log_info(logger, "RAX: %.*s", 16, registro.RAX);
    log_info(logger, "RBX: %.*s", 16, registro.RBX);
    log_info(logger, "RCX: %.*s", 16, registro.RCX);
    log_info(logger, "RDX: %.*s", 16, registro.RDX);
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
			pasar_a_cola_ready(pcb, logger);
			break;
		case PROCESO_FINALIZADO:
			log_info(logger, "P_CORTO -> Proceso desalojado por EXIT");
			pasar_a_cola_exit(pcb, logger, SUCCESS);
			break;
		case PROCESO_BLOQUEADO: //BLOQUEADO POR IO

			log_info(logger, "P_CORTO -> Proceso desalojado por BLOQUEO");
			log_info(logger, "PID: <%d> - Bloqueado por: <IO>",pcb->pid);
			sleep(2);
			int tiempo_bloqueo = recibir_operacion(socket_cpu);//CPU le manda el tiempo

			log_info(logger,"El proceso %d debera bloquearse por %d milisegundos",pcb->pid,tiempo_bloqueo);

			args->algoritmo = algoritmo;
			args->tiempo_bloqueo = tiempo_bloqueo;
			args->pcb = pcb;
			args->logger = logger;
			pcb->estimado_rafaga = pcb->nuevo_estimado;
			pcb->nuevo_estimado = 0;
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
			pcb->estimado_rafaga = pcb->nuevo_estimado;
			pcb->nuevo_estimado = 0;
			pthread_create(&thread_bloqueados, NULL, (void *)procesar_wait_recurso,(void*) args);
			pthread_detach(thread_bloqueados);
			break;

		case PROCESO_DESALOJADO_POR_SIGNAL:

			nombre = recibir_recurso();
			log_info(logger, "P_CORTO -> Proceso desalojado para ejecutar SIGNAL de %s ", nombre);
			args->algoritmo = algoritmo;
			args->nombre_recurso = nombre;
			args->pcb = pcb;
			args->logger = logger;
			pcb->estimado_rafaga = pcb->nuevo_estimado;
			pcb->nuevo_estimado = 0;
			pthread_create(&thread_bloqueados, NULL, (void *)procesar_signal_recurso, (void *)args);
			pthread_detach(thread_bloqueados);
			break;
		default:
			log_error(logger, "Error: La respuesta del CPU es innesperada. Cod: %d", cod_op);
			EXIT_FAILURE;
	}

}

void bloqueo_io(void* vArgs){

	t_args_hilo_block* args = (t_args_hilo_block*) vArgs;
	char* algoritmo = args->algoritmo;
	int tiempo = args->tiempo_bloqueo;
	t_log* logger = args->logger;
	t_pcb* pcb = args->pcb;

	log_info(logger,"PID: <%d> - Ejecuta IO: <%d>", pcb->pid, tiempo);
	pasar_a_cola_blocked(pcb, logger, colas_planificacion->cola_block);
	usleep(tiempo*1000000);
	pasar_segun_algoritmo(algoritmo,pcb,logger);
}

void procesar_wait_recurso(void* vArgs) {

	t_args_hilo_block* args = (t_args_hilo_block*) vArgs;
	char* nombre = args->nombre_recurso;
	t_log* logger = args->logger;
	t_pcb* pcb = args->pcb;
	char* algoritmo = args->algoritmo;

	int pos = buscar_recurso(nombre, logger);

	if(pos != -1 ){
		t_recurso* recurso = (t_recurso*)list_get(lista_recursos,pos);

		if(recurso->instancias > 0){
			recurso->instancias--;
			log_info(logger,"PID: <%d> - Wait: <%s> - Instancias: <%d>",pcb->pid,recurso->nombre,recurso->instancias);

			pasar_segun_algoritmo(algoritmo,pcb,logger);
		}
		else{
			pasar_a_cola_blocked(pcb, logger, recurso->cola_bloqueados);
			log_info(logger,"PID: <%d> - Bloqueado por: <%s>",pcb->pid,nombre);
		}
	}
	else{
		log_info(logger, "No se encontro recurso %s , pasando PROCESO <%d> a EXIT",nombre,pcb->pid);
		pasar_a_cola_exit(pcb, logger, RESOURCE_NOT_FOUND); //TODO
	}

	free(nombre);
}

void procesar_signal_recurso(void* vArgs){

	t_args_hilo_block* args = (t_args_hilo_block*) vArgs;
	char* nombre = args->nombre_recurso;
	t_log* logger = args->logger;
	t_pcb* proceso = args->pcb;
	char* algoritmo = args->algoritmo;

	int pos = buscar_recurso(nombre, logger);

	if(pos != -1){
		t_recurso* recurso = (t_recurso*)list_get(lista_recursos,pos);
		recurso->instancias++;
		log_info(logger,"PID: <%d> - Signal: <%s> - Instancias: <%d>",proceso->pid,nombre,recurso->instancias);

		if(queue_size(recurso->cola_bloqueados->cola) > 0){
			pasar_segun_algoritmo(algoritmo, squeue_pop(recurso->cola_bloqueados),logger);
		}

		pasar_segun_algoritmo(algoritmo,proceso,logger);
	}
	else{
		log_info(logger, "No se encontro recurso, pasando a EXIT");
		pasar_a_cola_exit(proceso, logger, RESOURCE_NOT_FOUND); //TODO
	}

	free(nombre);
}

void pasar_segun_algoritmo(char* algoritmo, t_pcb* proceso, t_log* logger){
	if(string_equals_ignore_case(algoritmo, "HRRN")) {
		pasar_a_cola_ready_en_orden(proceso, logger, comparador_hrrn);
	} else {
		pasar_a_cola_ready(proceso, logger);
	}
}

t_pcb* planificar(char* algoritmo, t_log* logger) {
	if (string_equals_ignore_case(algoritmo, "HRRN")) {
		//t_list* lista_ordenada = list_sorted(colas_planificacion->cola_ready->elements, (bool (*)(void *, void *))comparador_hrrn);
		//t_pcb* pcb = (t_pcb*)queue_peek(colas_planificacion->cola_ready);
		// "FIFO"
		// TODO: evaluar cola y tomar candidato
		//log_info(logger, "planificando con %s", algoritmo);
		t_pcb* pcb = proximo_proceso_hrrn(logger);
		//loggear_cola_ready(logger, algoritmo);
		log_info(logger,"PID:%d ",pcb->pid);
		pasar_a_cola_exec(pcb, logger);
		return pcb;
	} else if (string_equals_ignore_case(algoritmo, "FIFO")){
		t_pcb* pcb = (t_pcb*)squeue_peek(colas_planificacion->cola_ready);
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

t_pcb* proximo_proceso_hrrn(t_log* logger) {
	pthread_mutex_lock(colas_planificacion->cola_ready->mutex);
	t_list* lista_ready = colas_planificacion->cola_ready->cola->elements;
	pthread_mutex_unlock(colas_planificacion->cola_ready->mutex);
	loggear_cola_ready(logger, kernel_config->ALGORITMO_PLANIFICACION);
	if (list_size(lista_ready) > 1) {
		list_sort(lista_ready, comparador_hrrn);
	}
	t_pcb* pcb = squeue_peek(colas_planificacion->cola_ready);
	log_info(logger,"Candidato PID: %d", pcb->pid);
	loggear_cola_ready(logger, kernel_config->ALGORITMO_PLANIFICACION);
	return pcb;
}

bool comparador_hrrn(void* pcb1, void* pcb2) {
	//log_info(logger,"||||||||| COMPARADOR |||||||||");
	t_pcb* pcb_nuevo = (t_pcb*) pcb1;
	t_pcb* pcb_lista = (t_pcb*) pcb2;

	double S_pcb_nuevo = calcular_estimado_proxima_rafaga(pcb_nuevo, logger);
	double S_pcb_lista =  calcular_estimado_proxima_rafaga(pcb_lista, logger);

	//log_info(logger, "Estimado anterior: %d", pcb_nuevo->estimado_rafaga);
	//log_info(logger, "Estimado nuevo: %d", pcb_nuevo->nuevo_estimado);
	int64_t W_pcb_nuevo =  temporal_gettime(pcb_nuevo->tiempo_llegada);
	int64_t W_pcb_lista =  temporal_gettime(pcb_lista->tiempo_llegada);

	double ratio_pcb_nuevo = (S_pcb_nuevo + W_pcb_nuevo) / (double)S_pcb_nuevo;
	double ratio_pcb_lista = (S_pcb_lista + W_pcb_lista) / (double)S_pcb_lista;

	if(ratio_pcb_nuevo >= ratio_pcb_lista) {
//		log_info(logger, "HRRN -> PID %d - [S: %f] - [W: %d] - [RR: %f] > PID %d: - [S: %f] - [W: %d] - [RR:%f]" ,
//					pcb_nuevo->pid, S_pcb_nuevo, W_pcb_nuevo, ratio_pcb_nuevo,
//					pcb_lista->pid, S_pcb_lista, W_pcb_lista, ratio_pcb_lista);
		return true;
	} else {
//		log_info(logger, "HRRN -> PID %d - [S: %f] - [W: %d] - [RR: %f] < PID %d: - [S: %f] - [W: %d] - [RR:%f]"  ,
//					pcb_nuevo->pid, S_pcb_nuevo, W_pcb_nuevo, ratio_pcb_nuevo,
//					pcb_lista->pid, S_pcb_lista, W_pcb_lista, ratio_pcb_lista);
		return false;
	}
}

