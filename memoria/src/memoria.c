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
			case MEMORY_CREATE_SEGMENT:
				int id_crear = recibir_entero(socket_kernel);
				int tamanio = recibir_entero(socket_kernel);
				log_info(logger, "MEMORY_CREATE_SEGMENT tamanio %d",tamanio);
				crear_segmento(tamanio);
				loggear_segmentos();
				break;
			case MEMORY_DELETE_SEGMENT:
				int id_eliminar = recibir_entero(socket_kernel);
				log_info(logger, "MEMORY_DELETE_SEGMENT %d", id_eliminar);
				destroy_segmento(id_eliminar);
				loggear_segmentos();
				loggear_huecos();
				break;
			case -1:
				log_error(logger, "Se desconectó el cliente.");
				break;
			default:
				break;
		}
	}
}

void enviar_segmento(int socket_kernel, t_segmento *segmento_aux) {
	enviar_entero(socket_kernel, segmento_aux->segmento_id); // SEGMENTO_ID
	enviar_entero(socket_kernel, segmento_aux->inicio); // INICIO
	enviar_entero(socket_kernel, segmento_aux->tam_segmento); // TAMAÑO SEGMENTO
}

void enviar_tabla_segmento(int socket_kernel, t_tabla_segmento* tabla_segmento) {
	enviar_entero(socket_kernel, MEMORY_SEGMENT_CREATED);
	enviar_entero(socket_kernel, tabla_segmento->pid); // PID

	int cant_segmentos = list_size(tabla_segmento->tabla);
	for (int i = 0; i < cant_segmentos; ++i) { // TABLA DE SEGMENTOS
		t_segmento* segmento_aux = list_get(tabla_segmento->tabla, i);
		enviar_segmento(socket_kernel, segmento_aux);
	}
}
