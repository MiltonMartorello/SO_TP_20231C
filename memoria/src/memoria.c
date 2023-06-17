#include "../Include/memoria.h"

t_espacio_usuario* espacio_usuario;
t_memoria_config* memoria_config;

int main(void) {

	logger = iniciar_logger("memoria.log");

	log_info(logger, "MODULO MEMORIA");

	memoria_config = leer_config("memoria.config");
	//correr_servidor(logger, memoria_config->puerto_escucha);

	iniciar_estructuras();
	crear_segmento(memoria_config->tam_segmento_0);
	destroy_segmento(0); // TODO, INYECCIÓN DE DEPENDENCIAS.
	destroy_estructuras();
	return EXIT_SUCCESS;
}

t_segmento* crear_segmento(int tam_segmento) {
    t_segmento* segmento = malloc(sizeof(t_segmento));
    segmento->valor = malloc(tam_segmento);  // Este valor lo seteará CPU de ser necesario
    segmento->segmento_id = id++; // id autoincremental de sistema (Descriptor)
	segmento->tam_segmento = tam_segmento; // Con la base + el tamaño se calcula la posición final
	log_info(logger, "Creando segmento con tamaño %d" ,tam_segmento);

	t_hueco* hueco;

	if (id == 0) {
		hueco = list_get(espacio_usuario->huecos_libres, 0);
	} else {
		// @Mock
		//hueco = list_get(espacio_usuario->huecos_libres, 0); // TODO
		hueco = buscar_hueco(tam_segmento);

		if(hueco == NULL){

		}
	}
	segmento->inicio = hueco->inicio; // Desde donde empieza, en el caso del segmento_0 esta bien que sea 0. Sino es la base del hueco
	log_info(logger, "Encontrado hueco con piso %d y %d de espacio total", hueco->inicio, hueco->fin - hueco->inicio);
	// Dentro del choclo de espacio de usuario nos movemos hasta el inicio del hueco libre encontrado, desde ahí asignamos el segmento, con el tamaño recibido
	memcpy(espacio_usuario->espacio_usuario + hueco->inicio, segmento->valor, tam_segmento);
	log_info(logger, "Copiado Segmento a espacio de usuario");
	actualizar_hueco(hueco, hueco->inicio + tam_segmento, hueco->fin); // Actualizamos el piso del hueco al nuevo offset.
	// Agregamos el segmento a la lista de segmentos activos
	list_add(espacio_usuario->segmentos_activos, segmento);

	log_info(logger, "Creado Segmento %d", segmento->segmento_id);

	return segmento;
}

void destroy_segmento(int id) {
	// Lambda
    bool encontrar_por_id(void* elemento) {
        t_segmento* segmento = (t_segmento*)elemento;
        return segmento->segmento_id == id;
    }
    log_info(logger, "Eliminando segmento %d...", id);
    t_segmento* segmento = list_find(espacio_usuario->segmentos_activos, encontrar_por_id);
    if (segmento == NULL) {
		log_error(logger, "Error: No se encontró el segmento %d para eliminar", id);
		return;
	}
    int tamanio = segmento->tam_segmento;
    int inicio_segmento = segmento->segmento_id;

    consolidar(inicio_segmento,tamanio);

    free(segmento->valor);
    free(segmento);
    log_info(logger, "Eliminado Segmento %d de %d Bytes", id, tamanio);

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

void eliminar_hueco(t_hueco* hueco) {
	list_remove_element(espacio_usuario->huecos_libres, hueco);
	free(hueco);
}

t_hueco* crear_hueco(int inicio, int fin) {
	t_hueco* hueco = malloc(sizeof(t_hueco));
	hueco->inicio = inicio;
	hueco->fin = fin;
	log_info(logger, "Creado Hueco Libre de memoria de %d Bytes", fin-inicio);
	list_add(espacio_usuario->huecos_libres, hueco);
	// TODO CALCULAR CONSOLIDACIÓN
	return hueco;
}

void iniciar_estructuras(void) {
	espacio_usuario = malloc(sizeof(t_espacio_usuario));
	espacio_usuario->huecos_libres = list_create();
	list_add(espacio_usuario->huecos_libres, crear_hueco(0, memoria_config->tam_memoria)); // la memoria se inicia con un hueco global
	espacio_usuario->segmentos_activos = list_create();
    log_info(logger, "Tamaño de memoria: %d", memoria_config->tam_memoria);
    espacio_usuario->espacio_usuario = malloc(memoria_config->tam_memoria);

    if (espacio_usuario->espacio_usuario == NULL) {
        log_error(logger, "No se pudo asignar memoria para el espacio de usuario");
        // Aquí puedes manejar el error según sea necesario (liberar recursos, finalizar el programa, etc.)
    } else {
        int tamaño_asignado = memoria_config->tam_memoria;
        log_info(logger, "Iniciado espacio de usuario con %d bytes", tamaño_asignado);
    }
}

void destroy_estructuras(void) {
	list_destroy(espacio_usuario->huecos_libres);
	list_destroy(espacio_usuario->segmentos_activos);
	free(espacio_usuario->espacio_usuario);
	free(espacio_usuario);
}



t_memoria_config* leer_config(char *path) {
    t_config *config = iniciar_config(path);
    t_memoria_config* tmp = malloc(sizeof(t_memoria_config));

    tmp->puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    log_info(logger, "Puerto de escucha: %s", tmp->puerto_escucha);

    tmp->tam_memoria = config_get_int_value(config,"TAM_MEMORIA");
    log_info(logger, "Tamaño de memoria: %d", tmp->tam_memoria);

    tmp->tam_segmento_0 = config_get_int_value(config,"TAM_SEGMENTO_0");
    log_info(logger, "Tamaño de segmento 0: %d", tmp->tam_segmento_0);

    tmp->cant_segmentos = config_get_int_value(config,"CANT_SEGMENTOS");
    log_info(logger, "Cantidad de segmentos: %d", tmp->cant_segmentos);

    tmp->retardo_memoria = config_get_int_value(config,"RETARDO_MEMORIA");
    log_info(logger, "Retardo de memoria: %d", tmp->retardo_memoria);

    tmp->retardo_compactacion = config_get_int_value(config,"RETARDO_COMPACTACION");
    log_info(logger, "Retardo de compactación: %d", tmp->retardo_compactacion);

    tmp->algoritmo_asignacion = config_get_string_value(config,"ALGORITMO_ASIGNACION");
    log_info(logger, "Algoritmo de asignación: %s", tmp->algoritmo_asignacion);

    //config_destroy(config);
    return tmp;
}


void correr_servidor(t_log *logger, char *puerto) {

	int server_fd = iniciar_servidor(puerto);

	log_info(logger, "Iniciada la conexión de servidor de memoria: %d",server_fd);

	while(escuchar_clientes(server_fd,logger));

	liberar_conexion(server_fd);

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

int aceptar_cliente(int socket_servidor) {
	struct sockaddr_in dir_cliente;
	int tam_direccion = sizeof(struct sockaddr_in);

	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente,(void*) &tam_direccion);

	return socket_cliente;
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


