#include "../Include/memoria.h"

int main(void) {

	logger = iniciar_logger("memoria.log");

	log_info(logger, "MODULO MEMORIA");

	memoria_config = leer_config("memoria.config");

	iniciar_estructuras();
	crear_segmento(memoria_config->tam_segmento_0);
  
  correr_servidor(logger, memoria_config->puerto_escucha);
	//destroy_segmento(0); // TODO, INYECCIÓN DE DEPENDENCIAS.

	destroy_estructuras();
	return EXIT_SUCCESS;
}


void consolidar(int inicio, int tamanio) {

	bool inicio_contiguo(void* elem) {
		t_hueco* hueco = (t_hueco*) elem;
		return hueco->inicio == (inicio + tamanio);
	}

	bool fin_contiguo(void* elem) {
		t_hueco* hueco = (t_hueco*) elem;
		return hueco->fin == (inicio - 1);
	}

	t_hueco* hueco_derecho = list_find(espacio_usuario->huecos_libres,&inicio_contiguo);
	t_hueco* hueco_izquierdo = list_find(espacio_usuario->huecos_libres,&fin_contiguo);

	if(hueco_izquierdo != NULL){

		if(hueco_derecho != NULL){
			actualizar_hueco(hueco_izquierdo, hueco_izquierdo->inicio, hueco_derecho->fin);
//			hueco_izquierdo->fin = hueco_derecho->fin;
			eliminar_hueco(hueco_derecho);
		}
		else {
			actualizar_hueco(hueco_izquierdo, hueco_izquierdo->inicio, hueco_izquierdo->fin + tamanio);
//			hueco_izquierdo->fin += tamanio;
		}
	}
	else if(hueco_derecho != NULL){
		actualizar_hueco(hueco_derecho, inicio, hueco_derecho->fin);
//		hueco_derecho->inicio = inicio;
	}
	else{
		crear_hueco(inicio, inicio + tamanio - 1); //TODO REVISAR
	}

}

void actualizar_hueco(t_hueco* hueco, int nuevo_piso, int nuevo_fin) {
	//log_info(logger, "Actualizando piso de hueco de %d a %d", hueco->inicio, nuevo_piso);
	hueco->inicio = nuevo_piso;
	hueco->fin = nuevo_fin;
	log_info(logger, "Hueco Libre actualizado: [%d-%d]", hueco->inicio, hueco->fin);
	// Si el hueco quedó vacío. Lo eliminamos.
	if(hueco->inicio >= hueco->fin) {
		log_info(logger, "El hueco quedó vacío. Eliminado hueco...");
		eliminar_hueco(hueco);
	}
}

void procesar_cliente(void *args_hilo) {

	t_args_hilo_cliente *args = (t_args_hilo_cliente*) args_hilo;

	int socket_cliente = args->socket;
	t_log *logger = args->log;

	int modulo = recibir_operacion(socket_cliente);

	switch (modulo) {

	case CPU:
		log_info(logger, "CPU conectado.");
		enviar_mensaje("Hola CPU! -Memoria ", socket_cliente, logger);
		break;

	case KERNEL:
		log_info(logger, "Kernel conectado.");
		enviar_mensaje("Hola KERNEL! -Memoria ", socket_cliente, logger);
		procesar_kernel(socket_cliente);
		break;

	case FILESYSTEM:
		log_info(logger, "FileSystem conectado.");
		enviar_mensaje("Hola FILESYSTEM! -Memoria ", socket_cliente, logger);
		break;
	case -1:
		log_error(logger, "Se desconectó el cliente.");
		break;

	default:
		log_error(logger, "Codigo de operacion desconocido.");
		break;
	}

	free(args);
	return;
}

int escuchar_clientes(int server_fd, t_log *logger) {

	int cliente_fd = aceptar_cliente(server_fd);

	if (cliente_fd != -1) {
		pthread_t hilo;

		t_args_hilo_cliente *args = malloc(sizeof(t_args_hilo_cliente));

		args->socket = cliente_fd;
		args->log = logger;

		pthread_create(&hilo, NULL, (void*) procesar_cliente, (void*) args);
		pthread_detach(hilo);

		return 1;
	}

	return 0;
}

void procesar_kernel(int socket_kernel) {

	while(true) {
		int cod_op = recibir_operacion(socket_kernel);
		switch (cod_op) {
			case MEMORY_CREATE_TABLE:
				int pid = recibir_entero(socket_kernel);
				log_info(logger, "Recibido MEMORY_CREATE_TABLE para PID: %d", pid);
				t_tabla_segmento* tabla_segmento = create_tabla_segmento(pid);
				enviar_tabla_segmento(socket_kernel, tabla_segmento);
				break;
			case -1:
				log_error(logger, "Se desconectó el cliente.");
				break;
			default:
				break;
		}
	}
}

void enviar_tabla_segmento(int socket_kernel, t_tabla_segmento* tabla_segmento) {
	enviar_entero(socket_kernel, MEMORY_SEGMENT_CREATED);
	enviar_entero(socket_kernel, tabla_segmento->pid); // PID

	int cant_segmentos = list_size(tabla_segmento->tabla);
	for (int i = 0; i < cant_segmentos; ++i) { // TABLA DE SEGMENTOS
		t_segmento* segmento_aux = list_get(tabla_segmento->tabla, i);
		enviar_entero(socket_kernel, segmento_aux->segmento_id); // SEGMENTO_ID
		enviar_entero(socket_kernel, segmento_aux->inicio); // INICIO
		enviar_entero(socket_kernel, segmento_aux->tam_segmento); // TAMAÑO SEGMENTO
	}
}

t_hueco* buscar_hueco(int tamanio){

	char* algoritmo = memoria_config->algoritmo_asignacion;
	t_hueco* hueco;

	if(string_equals_ignore_case(algoritmo, "BEST")){
		hueco = buscar_hueco_por_best_fit(tamanio);
		printf("ALGORITMO BEST\n");
	}
	else if(string_equals_ignore_case(algoritmo, "FIRST")){
		hueco = buscar_hueco_por_first_fit(tamanio);
		printf("ALGORITMO FIRST\n");
	}
	else{
		hueco = buscar_hueco_por_worst_fit(tamanio);
		printf("ALGORITMO WORST\n");
	}
	return hueco;
}


t_list* filtrar_huecos_libres_por_tamanio(int tamanio){

	bool _func_aux(void* elemento){
		t_hueco* hueco = (t_hueco*) elemento;
		return (hueco->fin-hueco->inicio) >= tamanio;
	}
	//busco los huecos que tienen un tamaño mayor o igual al tamaño necesario
	return list_filter(espacio_usuario->huecos_libres,&_func_aux);
}

t_hueco* buscar_hueco_por_best_fit(int tamanio){

	//busco los huecos que tienen un tamaño mayor o igual al tamaño necesario
	t_list* huecos_candidatos = filtrar_huecos_libres_por_tamanio(tamanio);

	void* _fun_aux_2(void* elem1,void* elem2){
		t_hueco* hueco1 = (t_hueco*) elem1;
		t_hueco* hueco2 = (t_hueco*) elem2;
		int tamanio1 = hueco1->fin - hueco1->inicio;
		int tamanio2 = hueco2->fin - hueco2->inicio;

		if(tamanio1 < tamanio2 ){//TODO QUIZAS CONVIENE TENER UN CAMPO TAMANIO
			return hueco1;
		}

		return hueco2;
	}

	//de los huecos, elijo el que sea de menor tamaño
	return (t_hueco*) list_get_minimum(huecos_candidatos,&_fun_aux_2);

}

t_hueco* buscar_hueco_por_first_fit(int tamanio){

	return (t_hueco*) list_get(filtrar_huecos_libres_por_tamanio(tamanio), 0);
}

t_hueco* buscar_hueco_por_worst_fit(int tamanio){

	t_list* huecos_candidatos = filtrar_huecos_libres_por_tamanio(tamanio);

	void* _fun_aux_2(void* elem1,void* elem2){
		t_hueco* hueco1 = (t_hueco*) elem1;
		t_hueco* hueco2 = (t_hueco*) elem2;
		int tamanio1 = hueco1->fin - hueco1->inicio;
		int tamanio2 = hueco2->fin - hueco2->inicio;

		if(tamanio1 > tamanio2 ){//TODO QUIZAS CONVIENE TENER UN CAMPO TAMANIO
			return hueco1;
		}

		return hueco2;
	}

	//de los huecos, elijo el que sea de mayor tamaño
	return (t_hueco*) list_get_maximum(huecos_candidatos,&_fun_aux_2);
}
