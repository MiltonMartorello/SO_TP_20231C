#include "../Include/memoria.h"

int main(int argc, char **argv) {

	logger = iniciar_logger("memoria.log");

	log_info(logger, "MODULO MEMORIA");

	if(argc < 1) {
		printf("Falta path a archivo de configuración.\n");
		return EXIT_FAILURE;
	}
	/* -- INICIAR CONFIGURACIÓN -- */
	char* config_path = argv[1];
	memoria_config = leer_config(config_path);

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
		enviar_mensaje("Hola CPU! -Memoria ", socket_cliente);
		procesar_cpu_fs(socket_cliente, "CPU");
		break;

	case KERNEL:
		log_info(logger, "Kernel conectado.");
		enviar_mensaje("Hola KERNEL! -Memoria ", socket_cliente);
		procesar_kernel(socket_cliente);
		break;

	case FILESYSTEM:
		log_info(logger, "FileSystem conectado.");
		enviar_mensaje("Hola FILESYSTEM! -Memoria ", socket_cliente);
		procesar_cpu_fs(socket_cliente, "FS");
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
				recibir_entero(socket_kernel);//size_paquete
				pid = recibir_entero_2(socket_kernel);
				//log_info(logger, "Recibido MEMORY_CREATE_TABLE para PID: %d", pid);
				t_tabla_segmento* tabla_segmento = crear_tabla_segmento(pid);
				enviar_tabla_segmento(socket_kernel, tabla_segmento, MEMORY_SEGMENT_TABLE_CREATED);
				log_info(logger, "Creación de Proceso PID: <%d>", pid);
				break;
			case MEMORY_DELETE_TABLE:
				recibir_entero(socket_kernel);//size_paquete
				pid = recibir_entero_2(socket_kernel);
				//log_info(logger, "Recibido MEMORY_DELETE_TABLE para PID: %d", pid);
				t_tabla_segmento* tabla = encontrar_tabla_segmento_por_pid(pid);
				if (tabla == NULL) {
					enviar_entero(socket_kernel, MEMORY_ERROR_TABLE_NOT_FOUND);
					break;
				}
				//log_info(logger, "Encontrada Tabla a eliminar para PID: %d (%d Segmentos)",tabla->pid , tabla->tabla->elements_count);
				destroy_tabla_segmento(tabla);
				enviar_entero(socket_kernel, MEMORY_SEGMENT_TABLE_DELETED);
				log_info(logger, "Eliminación de Proceso PID: <%d>", pid);
				loggear_segmentos(espacio_usuario->segmentos_activos, logger);
				break;
			case MEMORY_CREATE_SEGMENT:
				recibir_entero(socket_kernel);//size_paquete
				pid = recibir_entero_2(socket_kernel);
				int id_crear = recibir_entero_2(socket_kernel);
				int tamanio = recibir_entero_2(socket_kernel);

				//log_info(logger, "MEMORY_CREATE_SEGMENT PID: %d, SEG_ID: %d [%d bytes]", pid, id_crear, tamanio);
				if (crear_segmento(pid, tamanio, id_crear) == NULL) {
					if(memoria_disponible() >= tamanio){
						log_info(logger, "No hay espacio contiguo, pero habra espacio despues de una compactacion");
						enviar_entero(socket_kernel, MEMORY_NEEDS_TO_COMPACT);
					}
					else{
						log_error(logger, "OUT_OF_MEMORY EXCEPTION -> Retornando a Kernel");
						enviar_entero(socket_kernel, MEMORY_ERROR_OUT_OF_MEMORY);
					}
				}
				else{
					enviar_tabla_actualizada(socket_kernel, pid, id_crear, MEMORY_SEGMENT_CREATED);
				}
				break;
			case MEMORY_DELETE_SEGMENT:
				recibir_entero(socket_kernel);//size_paquete
				pid = recibir_entero_2(socket_kernel);
				int id_eliminar = recibir_entero_2(socket_kernel);
				//log_info(logger, "MEMORY_DELETE_SEGMENT PID: %d, SEG_ID: %d", pid , id_eliminar);
				//loggear_segmentos(buscar_tabla_segmentos(pid)->tabla,logger);
				loggear_segmentos(espacio_usuario->segmentos_activos, logger);
				delete_segmento(pid, id_eliminar);
				enviar_tabla_actualizada(socket_kernel, pid, id_crear, MEMORY_SEGMENT_DELETED);
				loggear_segmentos(espacio_usuario->segmentos_activos, logger);
				break;
			case MEMORY_COMPACT:
				log_info(logger, "Solicitud de compactacion"); //LOG DE CATEDRA
				compactar_memoria();
				resultado_compactacion();
				//sleep(memoria_config->retardo_compactacion / 1000);
				enviar_procesos_actualizados(socket_kernel);
				break;
			default:
				log_error(logger, "Se desconectó el cliente. Cod: %d", cod_op);
				liberar_conexion(socket_kernel);
				return;
				break;
		}
	}
}

void procesar_cpu_fs(int socket, char* modulo) {

	while(1){
		validar_conexion(socket);
		int operacion = recibir_operacion(socket);
		int pid;
		int direccion_fisica;
		switch(operacion) {
		case MEMORY_READ_ADRESS:
			pid = recibir_entero(socket);
			direccion_fisica = recibir_entero(socket);
			int cant_bytes = recibir_entero(socket);
			char* valor_leido = leer_direccion(direccion_fisica, cant_bytes);
			char* valor_para_log = malloc(cant_bytes + 1);
			strncpy(valor_para_log,valor_leido, cant_bytes);
			valor_para_log[cant_bytes] = '\0';
			log_info(logger, "PID: <%d> - Acción: <LEER> - Dirección física: <%d> - Tamaño: <%d> - Origen: <%s>", pid, direccion_fisica, cant_bytes, modulo);
			log_info(logger, "Valor leido: _%s_", valor_para_log);
			enviar_mensaje(valor_leido, socket);
			free(valor_leido);
			break;
		case MEMORY_WRITE_ADRESS:
			pid = recibir_entero(socket);
			direccion_fisica = recibir_entero(socket);
			int tamanio = recibir_entero(socket);
			char* valor_a_escribir = recibir_string(socket);
			char* valor_log = malloc(cant_bytes + 1);
			strncpy(valor_log, valor_a_escribir, cant_bytes);
			valor_log[cant_bytes] = '\0';
			
			log_info(logger, "PID: <%d> - Acción: <ESCRIBIR> - Dirección física: <%d> - Tamaño: <%d> - Origen: <%s>", pid, direccion_fisica, tamanio, modulo);
			log_info(logger, "Valor a escribir : _%s_", valor_log);
			escribir_en_direccion(direccion_fisica, tamanio, valor_log, socket);
			break;
		default:
			log_info(logger, "No pude reconocer operacion de %s.", modulo);
			liberar_conexion(socket);
			return;
			break;
		}
	}
}

void enviar_tabla_actualizada(int socket_kernel, int pid, int segmento_id, int cod_op) {
	t_tabla_segmento* tabla_segmento_aux = malloc(sizeof(t_tabla_segmento));
	tabla_segmento_aux->tabla =  encontrar_tabla_segmentos(pid);
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

//armo un paquete con todaas las tablas actualizadas
void enviar_procesos_actualizados(int socket) {

	t_paquete* paquete = crear_paquete(MEMORY_COMPACT);
	t_buffer* buffer = crear_buffer();
	paquete->buffer = buffer;

	buffer->size = sizeof(int);
	buffer->stream = malloc(buffer->size);

	memcpy(buffer->stream, &tablas_segmentos->elements_count, sizeof(int)); //AGREGO CANTIDAD DE PROCESOS

	//por cada proceso agrego: PID + SIZE_SERIALIZACION_TABLA + STREAM_SERIALIZACION_TABLA
	void _serializar_tabla_proceso(void* elem) {
		t_tabla_segmento* proceso = (t_tabla_segmento*) elem;

		buffer->stream = realloc(buffer->stream , buffer->size + sizeof(int));
		memcpy(buffer->stream + buffer->size, &proceso->pid, sizeof(int));
		buffer->size += sizeof(int);

		t_buffer* buffer_proceso = serializar_tabla_segmentos(proceso->tabla);

		agregar_a_paquete(paquete, buffer_proceso->stream, buffer_proceso->size);

		free(buffer_proceso->stream);
		free(buffer_proceso);
	}

	list_iterate(tablas_segmentos, &_serializar_tabla_proceso);
	enviar_paquete(paquete, socket);
	eliminar_paquete(paquete);
}

