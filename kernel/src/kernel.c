#include "../include/kernel.h"
#include "../include/i_console.h"

int main(void) {

	logger = iniciar_logger("kernel.log");
	log_info(logger, "MODULO KERNEL");

	/* -- INICIAR CONFIGURACIÓN -- */
	t_config* config_kernel = iniciar_config("./kernel.config");
	cargar_config_kernel(config_kernel);

	/* -- CONEXIÓN CON CPU -- */
	socket_cpu = conectar_con_cpu();

    /* -- CONEXIÓN CON MEMORIA -- */
	socket_memoria = conectar_con_memoria();

    /* -- CONEXIÓN CON FILESYSTEM -- */
	socket_memoria = conectar_con_filesystem();

	/* -- INICIAR KERNEL COMO SERVIDOR DE CONSOLAS -- */
    socket_kernel = iniciar_servidor(kernel_config->PUERTO_ESCUCHA);
	log_info(logger, "Iniciada la conexión de Kernel como servidor: %d",socket_kernel);

    while(1) {

        log_info(logger, "Esperando un cliente nuevo de la consola...");
        int socket_consola = esperar_cliente(socket_kernel, logger);
        log_info(logger, "Entro una consola con el socket: %d", socket_consola);

		int modulo = recibir_operacion(socket_consola);

			switch (modulo) {
				case CONSOLA:
					enviar_mensaje("Hola Consola! Soy tu amigo el Kernel", socket_consola, logger);
					pthread_t* hilo;

					t_args_hilo_cliente* args = malloc(sizeof(t_args_hilo_cliente));

					args->socket = socket_kernel;
					args->log = logger;

					int hilo_return = pthread_create(hilo, NULL, (void*) procesar_consola, (void*) args);
					if (hilo_return != 0) {
						return -1;
					}
					pthread_detach(hilo);
					free(args);
					break;

				default:
					log_error(logger, "CÓDIGO DE OPERACIÓN DESCONOCIDO.");
					break;
			}
    }

	/* -- FINALIZAR PROGRAMA -- */
	finalizar_kernel(socket_kernel, logger, config_kernel);

	return EXIT_SUCCESS;
}



void cargar_config_kernel(t_config* config){

	kernel_config = malloc(sizeof(t_kernel_config));

	kernel_config->IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
	kernel_config->PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");
	kernel_config->IP_FILESYSTEM = config_get_string_value(config, "IP_FILESYSTEM");
	kernel_config->PUERTO_FILESYSTEM = config_get_string_value(config, "PUERTO_FILESYSTEM");
	kernel_config->IP_CPU = config_get_string_value(config, "IP_CPU");
	kernel_config->PUERTO_CPU = config_get_string_value(config, "PUERTO_CPU");
	kernel_config->PUERTO_ESCUCHA = config_get_string_value(config, "PUERTO_ESCUCHA");
	kernel_config->ALGORITMO_PLANIFICACION = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	kernel_config->ESTIMACION_INICIAL = config_get_int_value(config, "ESTIMACION_INICIAL");
	kernel_config->HRRN_ALFA = config_get_int_value(config, "HRRN_ALFA");
	kernel_config->GRADO_MAX_MULTIPROGRAMACION = config_get_int_value(config, "GRADO_MAX_MULTIPROGRAMACION");
	kernel_config->RECURSOS = config_get_array_value(config, "RECURSOS");
	kernel_config->INSTANCIAS_RECURSOS = config_get_array_value(config, "INSTANCIAS_RECURSOS");

    log_info(logger, "La configuración se cargó en la estructura 'kernel_config' ");

}

int conectar_con_memoria(){

	log_info(logger, "Iniciando la conexión con MEMORIA [IP %s] y [PUERTO:%s]", kernel_config->IP_MEMORIA, kernel_config->PUERTO_MEMORIA);

	socket_memoria = crear_conexion(kernel_config->IP_MEMORIA, kernel_config->PUERTO_MEMORIA);
	enviar_handshake(socket_memoria, KERNEL);
	recibir_operacion(socket_memoria);
	recibir_mensaje(socket_memoria, logger);
	return socket_memoria;
}

int conectar_con_cpu(){

	log_info(logger, "Iniciando la conexión con CPU [IP %s] y [PUERTO:%s]", kernel_config->IP_CPU, kernel_config->PUERTO_CPU);

	socket_cpu = crear_conexion(kernel_config->IP_CPU, kernel_config->PUERTO_CPU);
	enviar_handshake(socket_cpu, KERNEL);
	recibir_operacion(socket_cpu);
	recibir_mensaje(socket_cpu, logger);
	return socket_cpu;
}

int conectar_con_filesystem(){

	log_info(logger, "Iniciando la conexión con FILESYSTEM [IP %s] y [PUERTO:%s]", kernel_config->IP_FILESYSTEM, kernel_config->PUERTO_FILESYSTEM);

	int socket_filesystem = crear_conexion(kernel_config->IP_FILESYSTEM, kernel_config->PUERTO_FILESYSTEM);
	enviar_handshake(socket_filesystem, KERNEL);
	recibir_operacion(socket_filesystem);
	recibir_mensaje(socket_filesystem,logger);
	return socket_filesystem;

}

void finalizar_kernel(int socket_servidor, t_log* logger, t_config* config)
{
	liberar_conexion(socket_cpu);
	liberar_conexion(socket_memoria);
	liberar_conexion(socket_filesystem);
	liberar_conexion(socket_servidor);
	printf("KERNEL FINALIZADO \n");
}
