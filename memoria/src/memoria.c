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
    segmento->id = id++; // id autoincremental de sistema (Descriptor)
	segmento->tam_segmento = tam_segmento; // Con la base + el tamaño se calcula la posición final
	log_info(logger, "Creando segmento con tamaño %d" ,tam_segmento);

	t_hueco* hueco;

	if (id == 0) {
		hueco = list_get(espacio_usuario->huecos_libres, 0);
	} else {
		// @Mock
		hueco = list_get(espacio_usuario->huecos_libres, 0); // TODO
	}
	segmento->inicio = hueco->inicio; // Desde donde empieza, en el caso del segmento_0 esta bien que sea 0. Sino es la base del hueco
	log_info(logger, "Encontrado hueco con piso %d y %d de espacio total", hueco->inicio, hueco->fin - hueco->inicio);
	// Dentro del choclo de espacio de usuario nos movemos hasta el inicio del hueco libre encontrado, desde ahí asignamos el segmento, con el tamaño recibido
	memcpy(espacio_usuario->espacio_usuario + hueco->inicio, segmento->valor, tam_segmento);
	log_info(logger, "Copiado Segmento a espacio de usuario");
	actualizar_hueco(hueco, tam_segmento); // Actualizamos el piso del hueco al nuevo offset.
	// Agregamos el segmento a la lista de segmentos activos
	list_add(espacio_usuario->segmentos_activos, segmento);

	log_info(logger, "Creado Segmento %d", segmento->id);

	return segmento;
}


void destroy_segmento(int id) {
	// Lambda
    bool encontrar_por_id(void* elemento) {
        t_segmento* segmento = (t_segmento*)elemento;
        return segmento->id == id;
    }
    log_info(logger, "Eliminando segmento %d...", id);
    t_segmento* segmento = list_find(espacio_usuario->segmentos_activos, encontrar_por_id);
    if (segmento == NULL) {
		log_error(logger, "Error: No se encontró el segmento %d para eliminar", id);
		return;
	}
    int tamanio = segmento->tam_segmento;
    int inicio_segmento = segmento->id;
    free(segmento->valor);
    free(segmento);
    log_info(logger, "Eliminado Segmento %d de %d Bytes", id, tamanio);
    crear_hueco(inicio_segmento, inicio_segmento + tamanio);

}

void actualizar_hueco(t_hueco* hueco, int nuevo_piso) {
	//log_info(logger, "Actualizando piso de hueco de %d a %d", hueco->inicio, nuevo_piso);
	hueco->inicio = nuevo_piso;
	log_info(logger, "Hueco Libre actualizado: [%d-%d]", hueco->inicio, hueco->fin);
	// Si el hueco quedó vacío. Lo eliminamos.
	if(hueco->inicio >= hueco->fin) {
		log_info(logger, "El hueco quedó vacío. Eliminado hueco...");
		list_remove_element(espacio_usuario->huecos_libres, hueco);
		free(hueco);
	}
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

