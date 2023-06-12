#include "shared.h"
#include <errno.h>

t_log* logger; //TODO se necesita aca?

/*
 * SERVIDOR
 * */

int iniciar_servidor(char* puerto)
{
	int socket_servidor;
	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, puerto, &hints, &servinfo);

	// Creamos el socket de escucha del servidor
	socket_servidor = socket(
			servinfo->ai_family,
			servinfo->ai_socktype,
			servinfo-> ai_protocol);

	// Asociamos el socket a un puerto
	bind(socket_servidor,servinfo->ai_addr,servinfo->ai_addrlen);

	// Escuchamos las conexiones entrantes
	listen(socket_servidor,SOMAXCONN);

	freeaddrinfo(servinfo);
	//log_trace(logger, "Listo para escuchar a mi cliente");

	return socket_servidor;
}

int esperar_cliente(int socket_servidor, t_log* logger)
{
	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor,NULL,NULL);
	//log_info(logger, "Se conecto un cliente!");
	return socket_cliente;
}

void enviar_handshake(int socket, int operacion) {
    void *buffer = malloc(sizeof(int));
    memcpy(buffer, &operacion, sizeof(int));
    send(socket, buffer, sizeof(int), 0);

    free(buffer);
}

void enviar_entero(int socket, int operacion) {
    void *buffer = malloc(sizeof(int));
    memcpy(buffer, &operacion, sizeof(int));
    send(socket, buffer, sizeof(int), 0);

    free(buffer);
}

int recibir_operacion(int socket_cliente){
	int cod_op;
	if( recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) < 0) {
		close(socket_cliente);
	    fprintf(stderr, "recv: %s (%d)\n", strerror(errno), errno);
		return errno;
	}
	return cod_op;
}

void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;
	if (recv(socket_cliente, size, sizeof(int), MSG_WAITALL) < 0) {
		close(socket_cliente);
		fprintf(stderr, "recv: %s (%d)\n", strerror(errno), errno);
		return errno;
	}
	buffer = malloc(*size);
	if (recv(socket_cliente, buffer, *size, MSG_WAITALL) < 0) {
		close(socket_cliente);
	    fprintf(stderr, "recv: %s (%d)\n", strerror(errno), errno);
		return errno;
	}
	//printf("Recibido un buffer de %d bytes \n", *size);
	return buffer;
}

void recibir_mensaje(int socket_cliente,t_log* logger)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

t_list* recibir_paquete(int socket_cliente, t_log* logger)
{
	int size = 0;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "El buffer recibido tiene un tamaño de %d \n", size);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}


/*
 * CLIENTE
 * */

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

int crear_conexion(char *ip, char* puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.

	int socket_cliente = socket(
			server_info->ai_family,
			server_info->ai_socktype,
			server_info-> ai_protocol);

	if (connect(socket_cliente,
				server_info->ai_addr,
				server_info->ai_addrlen) == -1)
		printf("error");

	freeaddrinfo(server_info);

	return socket_cliente;
}

void enviar_mensaje(char* mensaje, int socket_cliente,  t_log* logger)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);
	send(socket_cliente, a_enviar, bytes, 0);
	free(a_enviar);
	eliminar_paquete(paquete);
}

t_buffer* crear_buffer(void)
{
	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = 0;
	buffer->stream = NULL;
	return buffer;
}

t_paquete* crear_paquete(int tipo)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = tipo;
	paquete->buffer = NULL;
	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
    // Cod Operacion + Size Paquete
    int bytes = paquete->buffer->size + 2 * sizeof(int);
//    printf("Bytes a enviar: %d\n", bytes);
    void* a_enviar = serializar_paquete(paquete, bytes);

    if (a_enviar == NULL)
    {
        printf("Error al serializar el paquete.\n");
        return; // O manejar el error de acuerdo a tu lógica
    }

    int bytes_enviados = send(socket_cliente, a_enviar, bytes, 0);

    if (bytes_enviados == -1)
    {
        printf("Error al enviar el paquete.\n");
        free(a_enviar);
        return; // O manejar el error de acuerdo a tu lógica
    }

//    printf("Bytes enviados: %d\n", bytes_enviados);
    free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}

t_config* iniciar_config(char* path)
{
	t_config* nuevo_config;
	if((nuevo_config = config_create(path)) == NULL) {
		printf("no pude cargar la config \n");
		exit(1);
	}
	return nuevo_config;
}

/*
 * GENERAL
 * */

t_log* iniciar_logger(char* path)
{

	t_log* nuevo_logger;
	char * nombre_log = string_replace(path, ".log", "");
	if((nuevo_logger = log_create(path, nombre_log, 1, LOG_LEVEL_INFO)) == NULL) {
		printf("No pude crear el logger \n");
		exit(1);
	}
	free(nombre_log);
	return nuevo_logger;
}

void terminar_programa(int conexion, t_log* logger, t_config* config)
{
	if(logger != NULL) {
		log_destroy(logger);
	}

	if(config != NULL) {
		config_destroy(config);
	}

	liberar_conexion(conexion);
}

t_programa* crear_programa(t_list* instrucciones) {
	t_programa* programa = malloc(sizeof(t_programa));
	programa->size = 0;
	programa->instrucciones = instrucciones;
//	printf("Size del programa %d\n" ,programa->size);
	return programa;
}

void programa_destroy(t_programa* programa) {

	if (programa->instrucciones != NULL)
		list_destroy(programa->instrucciones);
	free(programa);
}

int validar_conexion(int socket) {

	int optval;
	socklen_t optlen = sizeof(optval);
	int err = getsockopt(socket, SOL_SOCKET, SO_ERROR, &optval, &optlen);
	if (err == 0) {
	    if (optval != 0) {
	        // hay un error de conexión pendiente
	        fprintf(stderr, "Error de conexión pendiente: %s\n", strerror(optval));
	        return -1;
	    }
	} else {
	    // hubo un error al obtener el estado de la conexión
	    fprintf(stderr, "Error al obtener el estado de la conexión: %s\n", strerror(errno));
	    return -1;
	}
	return 1;
}

char* nombre_de_instruccion(int cod_op) {
	switch(cod_op) {
		case 1:
			return "SET";
			break;
		case 2:
			return "WAIT";
			break;
		case 3:
			return "SIGNAL";
			break;
		case 4:
			return "YIELD";
			break;
		case 5:
			return "I/O";
			break;
		case 6:
			return "F_OPEN";
			break;
		case 7:
			return "F_READ";
			break;
		case 8:
			return "F_WRITE";
			break;
		case 9:
			return "F_TRUNCATE";
			break;
		case 10:
			return "F_SEEK";
			break;
		case 11:
			return "F_CLOSE";
			break;
		case 12:
			return "CREATE_SEGMENT";
			break;
		case 13:
			return "DELETE_SEGMENT";
			break;
		case 14:
			return "MOV_IN";
			break;
		case 15:
			return "MOV_OUT";
			break;
		case 16:
			return "EXIT";
			break;
		default:
			printf("Error: Operación de instrucción desconocida");
			EXIT_FAILURE;
	}
	return NULL;
}
