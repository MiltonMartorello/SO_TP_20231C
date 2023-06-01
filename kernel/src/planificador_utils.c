#include "../include/planificador_utils.h"

 t_colas* colas_planificacion;
 sem_t sem_grado_multiprogramacion;
 sem_t sem_nuevo_proceso;

 sem_t sem_ready_proceso;
 sem_t sem_exec_proceso;
 sem_t sem_block_proceso;
 sem_t sem_exit_proceso;

 pthread_mutex_t mutex_cola_new;
 pthread_mutex_t mutex_cola_ready;
 pthread_mutex_t mutex_cola_exit;

 t_list* lista_recursos;

 double alfa = 0.5;

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
	sem_init(&sem_block_proceso, 0, 0);
	sem_init(&sem_exit_proceso, 0, 0);
	pthread_mutex_init(&mutex_cola_new, NULL);
	pthread_mutex_init(&mutex_cola_ready, NULL);
	pthread_mutex_init(&mutex_cola_exit, NULL);
}

void destroy_semaforos(void) {

	sem_destroy(&sem_grado_multiprogramacion);
	sem_destroy(&sem_nuevo_proceso);
	sem_destroy(&sem_ready_proceso);
	sem_destroy(&sem_exec_proceso);
	sem_destroy(&sem_block_proceso);
	sem_destroy(&sem_exit_proceso);
}

t_pcb* crear_pcb(t_programa*  programa, int pid_asignado) {
	t_temporal temporal;
	temporal.elapsed_ms = 0;
	temporal.status = TEMPORAL_STATUS_STOPPED;

	t_pcb* pcb = malloc(sizeof(t_pcb));
	pcb->instrucciones = programa->instrucciones;
	pcb->estado_actual = NEW;
	pcb->estimado_rafaga = 10000;
	pcb->nuevo_estimado = 0;
	pcb->pid = pid_asignado;
	pcb->program_counter = 0;
	pcb->registros = crear_registro();
	pcb->tabla_archivos_abiertos = list_create();
	pcb->tabla_segmento = list_create();
	pcb->tiempo_llegada = &temporal; // malloc(sizeof(t_temporal));
	pcb->tiempo_ejecucion = &temporal;
	pcb->motivo = NOT_DEFINED;
	return pcb;
}

void destroy_pcb(t_pcb* pcb) {
	list_destroy(pcb->instrucciones);
	list_destroy(pcb->tabla_archivos_abiertos);
	list_destroy(pcb->tabla_segmento);
	temporal_destroy(pcb->tiempo_llegada);
	temporal_destroy(pcb->tiempo_ejecucion);
	free(pcb);
}


void pasar_a_cola_ready(t_pcb* pcb, t_log* logger) {

	switch(pcb->estado_actual){
		case NEW:
			pthread_mutex_lock(&mutex_cola_new);
			queue_pop(colas_planificacion->cola_new);
			pthread_mutex_unlock(&mutex_cola_new);
			break;
		case EXEC:
			queue_pop(colas_planificacion->cola_exec);
			temporal_stop(pcb->tiempo_ejecucion);
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
	pcb->tiempo_llegada = temporal_create();
	pthread_mutex_lock(&mutex_cola_ready);
	queue_push(colas_planificacion->cola_ready,pcb);
	pthread_mutex_unlock(&mutex_cola_ready);
	log_info(logger, "Cambio de Estado: PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>", pcb->pid, estado_anterior, estado_string(pcb->estado_actual));
	loggear_cola_ready(logger);
	sem_post(&sem_ready_proceso);
}

void pasar_a_cola_ready_en_orden(t_pcb* pcb_nuevo, t_log* logger, int(*comparador)(t_pcb*, t_pcb*, t_log*)) {

	switch(pcb_nuevo->estado_actual){
		case NEW:
			pthread_mutex_lock(&mutex_cola_new);
			queue_pop(colas_planificacion->cola_new);
			pthread_mutex_unlock(&mutex_cola_new);
			break;
		case EXEC:
			queue_pop(colas_planificacion->cola_exec);
			temporal_stop(pcb_nuevo->tiempo_ejecucion);
			break;
		case BLOCK:
			queue_pop(colas_planificacion->cola_block);
			break;
		default:
			log_error(logger, "Error, no es un estado válido");
			EXIT_FAILURE;
	}

	char* estado_anterior = estado_string(pcb_nuevo->estado_actual);
	pcb_nuevo->estado_actual = READY;
	int index = -1;
	t_pcb* pcb_lista = NULL;

	pthread_mutex_lock(&mutex_cola_ready);
	if(queue_is_empty(colas_planificacion->cola_ready)) {
		//log_info(logger, "Cola de Ready Vacía. Pusheando");
		queue_push(colas_planificacion->cola_ready, pcb_nuevo);
	} else {
		//log_info(logger, "Calculando HRRN");
		t_list_iterator* iterador_pcbs = list_iterator_create(colas_planificacion->cola_ready->elements);
		while(list_iterator_has_next(iterador_pcbs)) {
			pcb_lista = (t_pcb*) list_iterator_next(iterador_pcbs);
			//log_info(logger, "iterando index %d", iterador_pcbs->index);
			if (comparador(pcb_nuevo, pcb_lista, logger) == 1) {
				index = iterador_pcbs->index;
				break;
			}
		}
		pcb_nuevo->tiempo_llegada = temporal_create();
		if (index >= 0) {
			list_add_in_index(colas_planificacion->cola_ready->elements, index, pcb_nuevo);
		} else {
			queue_push(colas_planificacion->cola_ready, pcb_nuevo);
		}

		list_iterator_destroy(iterador_pcbs);
	}
	pthread_mutex_unlock(&mutex_cola_ready);
	log_info(logger, "Cambio de Estado: PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>", pcb_nuevo->pid, estado_anterior, estado_string(pcb_nuevo->estado_actual));
	loggear_cola_ready(logger);
	sem_post(&sem_ready_proceso);
}

double calcular_estimado_proxima_rafaga (t_pcb* pcb, t_log* logger) {
	double real_anterior;
	double estimado_anterior;
	//.current.tv_sec = -1;
	if(pcb->estimado_rafaga <= 0) {
		estimado_anterior = 10000.0;
	} else {
		estimado_anterior = (double) pcb->estimado_rafaga;
	}

	// Si el tiempo es negativo significa que esto se inicializó pero nunca se ejecutó.
	if(pcb->tiempo_ejecucion->elapsed_ms <= 0) {
		real_anterior = 0.0;
	} else {
		real_anterior = (double) temporal_gettime(pcb->tiempo_ejecucion);
	}
	//log_info(logger, "Estimado Anterior de PID %d: %f", pcb->pid, estimado_anterior);
	//log_info(logger, "Real Anterior de PID %d: %f", pcb->pid, real_anterior);
	double nuevo_estimado = alfa * estimado_anterior + (1 - alfa) * real_anterior;
	//log_info(logger, "Calculado nuevo estimado de PID %d: %f", pcb->pid, nuevo_estimado);
	pcb->nuevo_estimado = nuevo_estimado;
	return nuevo_estimado;
}

int comparador_hrrn(t_pcb* pcb_nuevo, t_pcb* pcb_lista, t_log* logger) {
	//log_info(logger,"||||||||| COMPARADOR |||||||||");

	double S_pcb_nuevo = calcular_estimado_proxima_rafaga(pcb_nuevo, logger);
	double S_pcb_lista =  calcular_estimado_proxima_rafaga(pcb_lista, logger);

	int64_t W_pcb_nuevo =  0;
	int64_t W_pcb_lista =  temporal_gettime(pcb_lista->tiempo_llegada);

	double ratio_pcb_nuevo = (S_pcb_nuevo + W_pcb_nuevo) / (double)S_pcb_nuevo;
	double ratio_pcb_lista = (S_pcb_lista + W_pcb_lista) / (double)S_pcb_lista;

	log_info(logger, "P_CORTO -> Comparando Ratios: pcb1(pid %d) - [S: %f] - [W: %ld] - [RR: %f] ||| pcb2(pid %d): - [S: %f] - [W: %ld] - [RR:%f]" ,
			pcb_nuevo->pid, S_pcb_nuevo, W_pcb_nuevo, ratio_pcb_nuevo,
			pcb_lista->pid, S_pcb_lista, W_pcb_lista, ratio_pcb_lista);

	if(ratio_pcb_nuevo > ratio_pcb_lista) {
		log_info(logger, "P_CORTO -> pcb1(pid %d) - [RR: %f] > pcb2(pid %d): [RR:%f]" , pcb_nuevo->pid, ratio_pcb_nuevo, pcb_lista->pid, ratio_pcb_lista);
		return 1;
	} else if (ratio_pcb_nuevo  == ratio_pcb_lista) {
		// TODO es necesario?
		log_info(logger, "P_CORTO -> pcb1(pid %d) - [RR: %f] == pcb2(pid %d): [RR:%f]" , pcb_nuevo->pid, ratio_pcb_nuevo, pcb_lista->pid, ratio_pcb_lista);
		return 1;
	} else {
		log_info(logger, "P_CORTO -> pcb1(pid %d) - [RR: %f] < pcb2(pid %d): [RR:%f]" , pcb_nuevo->pid, ratio_pcb_nuevo, pcb_lista->pid, ratio_pcb_lista);
		return -1;
	}
}

void pasar_a_cola_exec(t_pcb* pcb,t_log* logger) {
	if(pcb->estado_actual != READY){
		log_error(logger, "Error, no es un estado válido");
		EXIT_FAILURE;
	}
	pthread_mutex_lock(&mutex_cola_ready);
	queue_pop(colas_planificacion->cola_ready);
	pthread_mutex_unlock(&mutex_cola_ready);
	char* estado_anterior = estado_string(pcb->estado_actual);
	pcb->estado_actual = EXEC;
	pcb->tiempo_ejecucion = temporal_create();
	temporal_stop(pcb->tiempo_llegada);
	pthread_mutex_lock(&mutex_cola_exit);
	queue_push(colas_planificacion->cola_exec, pcb);
	pthread_mutex_unlock(&mutex_cola_exit);
	log_info(logger, "Cambio de Estado: PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>", pcb->pid, estado_anterior, estado_string(pcb->estado_actual));
	sem_post(&sem_exec_proceso);
}

void pasar_a_cola_blocked(t_pcb* pcb, t_log* logger,t_queue* cola) {
	if(pcb->estado_actual != EXEC){
		log_error(logger, "Error, no es un estado válido");
		EXIT_FAILURE;
	}
	queue_pop(colas_planificacion->cola_exec);
	char* estado_anterior = estado_string(pcb->estado_actual);
	pcb->estado_actual = BLOCK;
	//queue_push(colas_planificacion->cola_block, pcb);
	queue_push(cola,pcb);
	log_info(logger, "Cambio de Estado: PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>", pcb->pid, estado_anterior, estado_string(pcb->estado_actual));
	sem_post(&sem_block_proceso);
}

void pasar_a_cola_exit(t_pcb* pcb, t_log* logger, return_code motivo) {
	if(pcb->estado_actual != EXEC){
		log_error(logger, "Error, no es un estado válido");
		EXIT_FAILURE;
	}
	queue_pop(colas_planificacion->cola_exec);
	char* estado_anterior = estado_string(pcb->estado_actual);
	pcb->estado_actual = EXIT;
	pcb->motivo = motivo;
	pthread_mutex_lock(&mutex_cola_exit);
	queue_push(colas_planificacion->cola_exit, pcb);
	pthread_mutex_unlock(&mutex_cola_exit);
	log_info(logger, "Cambio de Estado: PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>", pcb->pid, estado_anterior, estado_string(pcb->estado_actual));
	sem_post(&sem_exit_proceso);
}

void ejecutar_proceso(int socket_cpu, t_pcb* pcb, t_log* logger){
	log_info(logger,"P_CORTO -> Ejecutando PID: %d...",pcb->pid);
	t_contexto_proceso* contexto_pcb = malloc(sizeof(t_contexto_proceso));
	contexto_pcb->pid = pcb->pid;
	contexto_pcb->program_counter = pcb->program_counter;
	contexto_pcb->instrucciones = pcb->instrucciones;
	contexto_pcb->registros = pcb->registros;
	log_info(logger,"El pcb tiene el PC en %d ",pcb->program_counter);
	log_info(logger,"El pcb tiene %d instrucciones",list_size(pcb->instrucciones));
	//log_info(logger,"Voy a ejecutar proceso de %d instrucciones", list_size(contexto_pcb->instrucciones));
	enviar_contexto(socket_cpu, contexto_pcb, CONTEXTO_PROCESO, logger);

	free(contexto_pcb);
}

void loggear_cola_ready(t_log* logger) {
    char* pids = concatenar_pids(colas_planificacion->cola_ready->elements);

    log_info(logger, "P_CORTO -> Cola Ready <HRRN>: [%s]", pids);
    free(pids);
}

char* concatenar_pids(t_list* lista) {
    char* pids = string_new();

    void concatenar_pid(void* elemento) {
        int pid = *((int*)elemento);

        char* pid_str = string_itoa(pid);
        string_append_with_format(&pids, "%s, ", pid_str);
        free(pid_str);
    }

    list_iterate(lista, concatenar_pid);

    if (string_length(pids) > 0) {
        pids = string_substring_until(pids, string_length(pids) - 2);
    }

    return pids;
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

//TODO revisar
t_registro crear_registro(void) {

	t_registro registro;
	return registro;
}

t_temporal* temporal_reset(t_temporal* temporal) {
	if (temporal != NULL) {
		temporal_destroy(temporal);
	}
	temporal = temporal_create();
	return temporal;
}

void iniciar_recursos(char** recursos, char** instancias){

	lista_recursos = list_create();

	for(int i=0;i< string_array_size(recursos);i++){

		t_recurso* recurso = malloc(sizeof(t_recurso));
		recurso->nombre = recursos[i];
		recurso->instancias = instancias[i];
		recurso->cola_bloqueados = queue_create();

		list_add(lista_recursos, recurso);
	}
}

t_recurso* buscar_recurso(char* nombre){

	bool _func_aux(t_recurso* recurso){

		return string_equals_ignore_case(recurso->nombre, nombre);
	}

	return (t_recurso*)list_find(lista_recursos,_func_aux);
}
