#include "../Include/memoria.h"

t_espacio_usuario* espacio_usuario;
t_memoria_config* memoria_config;

int main(void) {

	t_log *logger = iniciar_logger("memoria.log");

	log_info(logger, "MODULO MEMORIA");

	memoria_config = leer_config("memoria.config");

	correr_servidor(logger, memoria_config->puerto_escucha);

	iniciar_estructuras();

	return EXIT_SUCCESS;
}


void iniciar_estructuras () {

	espacio_usuario->espacio_usuario = malloc(memoria_config->tam_memoria);
	log_info(logger, "Iniciado espacio de usuario con %d bytes", sizeof(espacio_usuario->espacio_usuario));


}


t_memoria_config* leer_config(char *path) {

	t_config *config = iniciar_config(path);
	t_memoria_config* tmp = malloc(sizeof(t_memoria_config));

	tmp->puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	tmp->tam_memoria = config_get_int_value(config,"TAM_MEMORIA");
	tmp->tam_segmento_0 = config_get_int_value(config,"TAM_SEGMENTO_0");
	tmp->cant_segmentos = config_get_int_value(config,"CANT_SEGMENTOS");
	tmp->retardo_memoria = config_get_int_value(config,"RETARDO_MEMORIA");
	tmp->retardo_compactacion = config_get_int_value(config,"RETARDO_COMPACTACION");
	tmp->algoritmo_asignacion = config_get_int_value(config,"ALGORITMO_ASIGNACION");

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

