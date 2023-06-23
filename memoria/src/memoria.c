#include "../Include/memoria.h"

int main(void) {

	logger = iniciar_logger("memoria.log");

	log_info(logger, "MODULO MEMORIA");

	memoria_config = leer_config("memoria.config");

	iniciar_estructuras();
	crear_segmento(SEGMENTO_0, memoria_config->tam_segmento_0, 0);
	correr_servidor(logger, memoria_config->puerto_escucha);

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
		//procesar_pedidos_cpu(socket_cliente);
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
	int pid;

	while(true) {
		validar_conexion(socket_kernel);
		int cod_op = recibir_operacion(socket_kernel);
		switch (cod_op) {
			case MEMORY_CREATE_TABLE:
				pid = recibir_entero(socket_kernel);
				log_info(logger, "Recibido MEMORY_CREATE_TABLE para PID: %d", pid);
				t_tabla_segmento* tabla_segmento = crear_tabla_segmento(pid);
				enviar_tabla_segmento(socket_kernel, tabla_segmento, MEMORY_SEGMENT_TABLE_CREATED);
				break;
			case MEMORY_DELETE_TABLE:
				pid = recibir_entero(socket_kernel);
				log_info(logger, "Recibido MEMORY_DELETE_TABLE para PID: %d", pid);
				t_tabla_segmento* tabla = encontrar_tabla_segmento_por_pid(pid);
				if (tabla == NULL) {
					enviar_entero(socket_kernel, MEMORY_ERROR_TABLE_NOT_FOUND);
					break;
				}
				log_info(logger, "Encontrada Tabla a eliminar para PID: %d (%d Segmentos)",tabla->pid , tabla->tabla->elements_count);
				destroy_tabla_segmento(tabla);
				enviar_entero(socket_kernel, MEMORY_SEGMENT_TABLE_DELETED);
				break;
			case MEMORY_CREATE_SEGMENT:
				pid = recibir_entero(socket_kernel);
				int id_crear = recibir_entero(socket_kernel);
				int tamanio = recibir_entero(socket_kernel);

				log_info(logger, "MEMORY_CREATE_SEGMENT PID: %d, SEG_ID: %d [%d bytes]", pid, id_crear, tamanio);
				crear_segmento(pid, tamanio, id_crear);
				enviar_tabla_actualizada(socket_kernel, pid, id_crear, MEMORY_SEGMENT_CREATED);
				break;
			case MEMORY_DELETE_SEGMENT:
				pid = recibir_entero(socket_kernel);
				int id_eliminar = recibir_entero(socket_kernel);
				log_info(logger, "MEMORY_DELETE_SEGMENT PID: %d, SEG_ID: %d", pid , id_eliminar);
				delete_segmento(pid, id_eliminar);
				enviar_tabla_actualizada(socket_kernel, pid, id_crear, MEMORY_SEGMENT_DELETED);
				break;
			default:
				log_error(logger, "Se desconectó el cliente. Cod: %d", cod_op);
				liberar_conexion(socket_kernel);
				return;
				break;
		}
	}
}



void procesar_pedidos_cpu(int socket_cpu) {

	while(1){
		int operacion = recibir_operacion(socket_cpu);
		int direccion_fisica;
//		switch(operacion) {
//		case LEER_DIRECCION:
//			direccion_fisica = recibir_entero(socket_cpu);
//			char* valor_leido = leer_direccion(direccion_fisica);
//			enviar_mensaje(valor_leido, socket_cpu, logger);
//			break;
//		case ESCRIBIR_DIRECCION:
//			direccion_fisica = recibir_entero(socket_cpu);
//			char* valor_a_escribir = recibir_string(socket_cpu);
//			escribir_en_direccion(direccion_fisica, valor_a_escribir);
//			break;
//		default:d
//		log_info(logger, "No pude reconocer operacion que CPU me mando.");
//		break;
//		}
	}
}



void enviar_tabla_actualizada(int socket_kernel, int pid, int segmento_id, int cod_op) {
	t_tabla_segmento* tabla_segmento_aux = malloc(sizeof(t_tabla_segmento));
	tabla_segmento_aux->tabla =  encontrar_tabla_segmentos(pid, segmento_id);
	tabla_segmento_aux->pid = pid;
	enviar_tabla_segmento(socket_kernel, tabla_segmento_aux, cod_op);
	free(tabla_segmento_aux);
}

void enviar_tabla_segmento(int socket_kernel, t_tabla_segmento* tabla_segmento, int cod_op) {

	// COD OP
	enviar_entero(socket_kernel, cod_op);

	t_buffer* buffer = serializar_tabla_segmentos(tabla_segmento->tabla);
	// PID +  SIZE + TABLA/BUFFER
	int bytes = sizeof(int) + sizeof(int) + buffer->size;
	void* magic = malloc(bytes);
	int offset = 0;

	// PID
	memcpy(magic, &(tabla_segmento->pid), sizeof(int));
	memcpy(magic + offset, buffer->stream, buffer->size);
	offset += sizeof(int);

	// BUFFER SIZE
	memcpy(magic+ offset, &(buffer->size), sizeof(int));
	offset += sizeof(int);

	memcpy(magic + offset, buffer->stream, buffer->size);
	offset += buffer->size;

	send(socket_kernel, magic, bytes, 0);

	free(magic);
	buffer_destroy(buffer);

}


