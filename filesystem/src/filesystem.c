#include "../Include/filesystem.h"

int main(void) {

	char* ip;
	char* puerto_memoria;
	char* puerto_file_system;
	int socket_kernel;

	int socket_file_system;
	pthread_t * thread_servidor;

	t_log* logger = iniciar_logger("file_system.log");
	log_info(logger, "Log inicializado");

	t_config* config = iniciar_config(PATH_CONFIG);
	log_info(logger, "Config abierta desde %s", config->path);

	ip = config_get_string_value(config,"IP_MEMORIA");
	puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");
	puerto_file_system = config_get_string_value(config,"PUERTO_ESCUCHA");
	log_info(logger, "IP: %s.",ip);
	log_info(logger, "Puerto de conexión FILESYSTEM-MEMORIA: %s", puerto_memoria);
	log_info(logger, "Puerto de conexión KERNEL-FILESYSTEM: %s", puerto_file_system);

	socket_file_system = iniciar_servidor(puerto_file_system);
	log_info(logger, "Iniciada la conexión de servidor de file system: %d", socket_file_system);

	conexion_con_memoria(ip, puerto_memoria,logger);

	socket_kernel = pthread_create(&thread_servidor, NULL, esperar_cliente_hilo,  (void *) &socket_file_system);
	int cod = pthread_join(thread_servidor, NULL);
	log_info(logger, "el codigo es %d", cod);
	terminar_programa(socket_kernel,logger,config);
	liberar_conexion(socket_file_system);
	return EXIT_SUCCESS;
}

void conexion_con_memoria(char* ip,char* puerto,t_log* logger){
	int socket_memoria = crear_conexion(ip, puerto);
	//enviar_mensaje("Hola Memoria desde el File System", socket_memoria, logger);

	enviar_handshake(socket_memoria,FILESYSTEM);
	recibir_operacion(socket_memoria);
	recibir_mensaje(socket_memoria,logger);
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

void* esperar_cliente_hilo (void *arg){
	int socket_file_system = *(int *)arg;
	printf("hola soy un hilo en el socket %d", socket_file_system);
	int socket_kernel = esperar_cliente(socket_file_system,logger);
	log_info(logger, "El Kernel se conectó con el socket %d",socket_kernel);
	return (void*) socket_kernel;

}
