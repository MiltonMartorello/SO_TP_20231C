#include "../Include/memoria_utils.h"

t_espacio_usuario* espacio_usuario;
t_memoria_config* memoria_config;
t_list* tablas_segmentos;

t_log * logger;
int id = 0;

void iniciar_estructuras(void) {
	espacio_usuario = malloc(sizeof(t_espacio_usuario));
	espacio_usuario->huecos_libres = list_create();
	crear_hueco(0, memoria_config->tam_memoria - 1); // la memoria se inicia con un hueco global
	espacio_usuario->segmentos_activos = list_create();
    log_info(logger, "Tamaño de memoria: %d", memoria_config->tam_memoria);
    espacio_usuario->espacio_usuario = malloc(memoria_config->tam_memoria);

    if (espacio_usuario->espacio_usuario == NULL) {
        log_error(logger, "No se pudo asignar memoria para el espacio de usuario");
        EXIT_FAILURE;
        return;
    } else {
        int tamaño_asignado = memoria_config->tam_memoria;
        log_info(logger, "Iniciado espacio de usuario con %d bytes", tamaño_asignado);
    }

    tablas_segmentos = list_create();
}

void destroy_estructuras(void) {
	list_destroy(espacio_usuario->huecos_libres);
	list_destroy(espacio_usuario->segmentos_activos);
	list_destroy_and_destroy_elements(tablas_segmentos, destroy_tabla_segmento); //TODO AND DESTROY ELEMENTS
	//delete_segmento(SEGMENTO_0, SEGMENTO_0); //TODO already free en la linea de arriba, pero revisar.
	free(espacio_usuario->espacio_usuario);
	free(espacio_usuario);
}

t_segmento* crear_segmento(int pid, int tam_segmento, int segmento_id) {
    t_segmento* segmento = malloc(sizeof(t_segmento));
    //segmento->valor = malloc(tam_segmento);  // Este valor lo seteará CPU de ser necesario
    segmento->segmento_id = segmento_id; // id autoincremental de sistema (Descriptor)
	segmento->tam_segmento = tam_segmento; // Con la base + el tamaño se calcula la posición final
	log_info(logger, "Creando segmento con tamaño %d" ,tam_segmento);

	t_hueco* hueco;

	if (pid == SEGMENTO_0) { // SI EL ID ES 0 => SEGMENTO_O, ELSE ES UN PROCESO. PID siempre > 0
		hueco = list_get(espacio_usuario->huecos_libres, 0);
	} else {
		t_tabla_segmento* tabla_segmento = buscar_tabla_segmentos(pid);
//		log_info(logger, "Tengo una tabla de segmentos con PID %d y %d segments", tabla_segmento->pid, tabla_segmento->tabla->elements_count);
//		log_info(logger,"Tengo un segmento de id %d y tamaño %d" , segmento->segmento_id, segmento->tam_segmento);
		list_add(tabla_segmento->tabla, segmento);
		hueco = buscar_hueco(tam_segmento);
	}
	segmento->inicio = hueco->inicio; // Desde donde empieza, en el caso del segmento_0 esta bien que sea 0. Sino es la base del hueco
	log_info(logger, "Encontrado hueco con piso %d y %d de espacio total", hueco->inicio, tamanio_hueco(hueco));
	// Dentro del choclo de espacio de usuario nos movemos hasta el inicio del hueco libre encontrado, desde ahí asignamos el segmento, con el tamaño recibido
	//memcpy(espacio_usuario->espacio_usuario + hueco->inicio, segmento->valor, tam_segmento);
	//log_info(logger, "Copiado Segmento a espacio de usuario");
	actualizar_hueco(hueco,hueco->inicio + tam_segmento, hueco->fin); // Actualizamos el piso del hueco al nuevo offset.
	// Agregamos el segmento a la lista de segmentos activos
	list_add(espacio_usuario->segmentos_activos, segmento);

	log_info(logger, "Creado Segmento %d", segmento->segmento_id);

	return segmento;
}

void delete_segmento(int pid, int segmento_id) {
	// Lambda
    bool encontrar_por_id(void* elemento) {
        t_segmento* segmento = (t_segmento*)elemento;
        return segmento->segmento_id == segmento_id;
    }
    log_info(logger, "Eliminando segmento %d...", segmento_id);
    t_segmento* segmento = list_find(espacio_usuario->segmentos_activos, encontrar_por_id);
    if (segmento == NULL) {
		log_error(logger, "Error: No se encontró el segmento %d para eliminar", segmento_id);
		return;
	}
    int tamanio = segmento->tam_segmento;
    int inicio_segmento = segmento->inicio;
    //free(segmento->valor);
    consolidar(inicio_segmento,tamanio);
    if (list_remove_element(espacio_usuario->segmentos_activos, segmento)) {
		free(segmento);
		log_info(logger, "Eliminado Segmento %d de %d Bytes", segmento_id, tamanio);
    }
}

t_hueco* crear_hueco(int inicio, int fin) {
	t_hueco* hueco = malloc(sizeof(t_hueco));
	hueco->inicio = inicio;
	hueco->fin = fin;
	log_info(logger, "Creado Hueco Libre de memoria de %d Bytes", tamanio_hueco(hueco));
	list_add(espacio_usuario->huecos_libres, hueco);
	return hueco;
}

void actualizar_hueco(t_hueco* hueco, int nuevo_piso, int nuevo_fin) {
	//log_info(logger, "Actualizando piso de hueco de %d a %d", hueco->inicio, nuevo_piso);
	hueco->inicio = nuevo_piso;
	hueco->fin = nuevo_fin;
	// Si el hueco quedó vacío. Lo eliminamos.
	if(hueco->inicio > hueco->fin) { // si el inicio y el fin son iguales es porque es de tamaño 1
		log_info(logger, "El hueco quedó vacío. Eliminado hueco...");
		eliminar_hueco(hueco);
	}
	else{
		log_info(logger, "Hueco Libre actualizado: [%d-%d]", hueco->inicio, hueco->fin);
	}
}

void eliminar_hueco(t_hueco* hueco) {
	list_remove_element(espacio_usuario->huecos_libres, hueco);
	free(hueco);
}

t_tabla_segmento* crear_tabla_segmento(int pid) {
	t_tabla_segmento* tabla_segmento  = malloc(sizeof(t_tabla_segmento));
	tabla_segmento->tabla = list_create();
	tabla_segmento->pid = pid;

	list_add(tabla_segmento->tabla, list_get(espacio_usuario->segmentos_activos, SEGMENTO_0));
	log_info(logger, "Creada tabla de segmentos para PID %d con %d segmentos", pid,  tabla_segmento->tabla->elements_count);
	list_add(tablas_segmentos, tabla_segmento);
	log_info(logger, "Tablas totales de segmentos: %d", tablas_segmentos->elements_count);
	return tabla_segmento;
}

t_tabla_segmento* buscar_tabla_segmentos(int pid) {
    bool encontrar_por_pid(void* tabla) {
        t_tabla_segmento* tabla_segmento = (t_tabla_segmento*)tabla;
        return tabla_segmento->pid == pid;
    }

    t_tabla_segmento* tabla = (t_tabla_segmento*)list_find(tablas_segmentos, encontrar_por_pid);

    if (tabla == NULL) {
        log_error(logger, "No se encontró la tabla de segmentos para el PID %d", pid);
    } else {
    	log_info(logger, "Encontrada tabla de segmentos para el PID %d con %d segmentos", pid, list_size(tabla->tabla));
    }
    loggear_tablas_segmentos();

    return tabla;
}

void loggear_tablas_segmentos(void) {
	log_info(logger, "Loggeando tabla de segmentos...");
	for (int i = 0; i < tablas_segmentos->elements_count; i++) {
		t_tabla_segmento* tabla_segmentos = list_get(tablas_segmentos, i);
		log_info(logger, "MEMCHECK -> Tabla de PID: %d", tabla_segmentos->pid);

		for (int j = 0; j < tabla_segmentos->tabla->elements_count; j++) {
			t_segmento* segmento = list_get(tabla_segmentos->tabla, j);
			log_info(logger, "MEMCHECK -> Segmento ID: %d", segmento->segmento_id);
			log_info(logger, "MEMCHECK -> Inicio: %d", segmento->inicio);
			log_info(logger, "MEMCHECK -> Tamaño: %d", segmento->tam_segmento);
		}
	}
}

void destroy_tabla_segmento(void* elemento) {
	t_tabla_segmento* tabla_segmento = (t_tabla_segmento*)elemento;
	int pid = tabla_segmento->pid;
    bool encontrar_por_pid(void* elemento) {
        t_tabla_segmento* tabla_segmento = (t_tabla_segmento*)elemento;
        return tabla_segmento->pid == pid;
    }

    t_tabla_segmento* tabla = (t_tabla_segmento*)list_find(tablas_segmentos, encontrar_por_pid);

    if (tabla == NULL) {
    	log_error(logger, "No se encontró la tabla de segmentos a eliminar para el PID %d\n", pid);
        return;
    }

    list_destroy_and_destroy_elements(tabla->tabla, free); // Liberar la memoria de los segmentos en la tabla
    list_remove_and_destroy_by_condition(tablas_segmentos, encontrar_por_pid, free); // Remover y liberar la memoria de la tabla
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

int aceptar_cliente(int socket_servidor) {
	struct sockaddr_in dir_cliente;
	int tam_direccion = sizeof(struct sockaddr_in);

	int socket_cliente = accept(socket_servidor, (void*) &dir_cliente,(void*) &tam_direccion);

	return socket_cliente;
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
			log_info(logger,"Tengo ambos vecinos :')");
			actualizar_hueco(hueco_izquierdo, hueco_izquierdo->inicio, hueco_derecho->fin);
			eliminar_hueco(hueco_derecho);
		}
		else {
			log_info(logger, "Tengo vecino izquierdo, Haremos fusion");
			actualizar_hueco(hueco_izquierdo, hueco_izquierdo->inicio, hueco_izquierdo->fin + tamanio);
		}
	}
	else if(hueco_derecho != NULL){
		log_info(logger, "Tengo vecino derecho, Haremos fusion");
		actualizar_hueco(hueco_derecho, inicio, hueco_derecho->fin);
	}
	else{
		log_info(logger,"No tengo vecinos :(");
		crear_hueco(inicio, inicio + tamanio - 1); //TODO REVISAR
	}
}

t_hueco* buscar_hueco(int tamanio){

	char* algoritmo = memoria_config->algoritmo_asignacion;
	t_hueco* hueco;

	if(string_equals_ignore_case(algoritmo, "BEST")){
		hueco = buscar_hueco_por_best_fit(tamanio);
	}
	else if(string_equals_ignore_case(algoritmo, "FIRST")){
		hueco = buscar_hueco_por_first_fit(tamanio);
	}
	else{
		hueco = buscar_hueco_por_worst_fit(tamanio);
	}
	return hueco;
}

t_list* filtrar_huecos_libres_por_tamanio(int tamanio){

	bool _func_aux(void* elemento){
		t_hueco* hueco = (t_hueco*) elemento;
		return tamanio_hueco(hueco) >= tamanio;
	}
	return list_filter(espacio_usuario->huecos_libres,&_func_aux);
}

t_hueco* buscar_hueco_por_best_fit(int tamanio){

	t_list* huecos_candidatos = filtrar_huecos_libres_por_tamanio(tamanio);

	void* _fun_aux_2(void* elem1,void* elem2){
		t_hueco* hueco1 = (t_hueco*) elem1;
		t_hueco* hueco2 = (t_hueco*) elem2;

		if(tamanio_hueco(hueco1) < tamanio_hueco(hueco2) ){
			return hueco1;
		}
		return hueco2;
	}
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

		if(tamanio_hueco(hueco1) > tamanio_hueco(hueco2) ){//TODO CUAL DEVOLVER SI SON IGUALES?
			return hueco1;
		}
		return hueco2;
	}
	return (t_hueco*) list_get_maximum(huecos_candidatos,&_fun_aux_2);
}

int tamanio_hueco(t_hueco* hueco){
	return hueco->fin - hueco->inicio + 1 ;
}

void loggear_huecos(t_list* huecos){
	log_info(logger,"----------HUECOS----------");
	log_info(logger,"HUECO_INICIO	HUECO_FIN");
	void _log(void* elem){
		t_hueco* hueco  = (t_hueco*) elem;

		log_info(logger,"	%d	%d",hueco->inicio,hueco->fin);
	}
	list_iterate(huecos,&_log);
}


