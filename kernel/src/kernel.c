#include "../include/kernel.h"

void conexion_con_memoria(char* ip,char* puerto,t_log* logger);

int main(void) {

	t_config* config;
	char* ip;
	char* puerto_memoria;

	t_log* logger;
	if((logger = log_create("tp0.log","TP0",1,LOG_LEVEL_INFO)) == NULL) {
		printf("no pude crear el logger \n");
		exit(1);
	}

	config = iniciar_config("./kernel.config");

	ip = config_get_string_value(config,"IP_MEMORIA");
	puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");

	conexion_con_memoria(ip,puerto_memoria,logger);

	log_info(logger, "test de log de kernel");
	int socket_memoria = iniciar_servidor(puerto_memoria);
	log_info(logger, "Iniciada la conexión de servidor de kernel: %d",socket_memoria);

	return EXIT_SUCCESS;
}


void cargar_config_kernel(t_config* config){

	config_kernel = malloc(sizeof(t_kernel_config));

	config_kernel->IP_MEMORIA 					= config_get_string_value(config, "IP_MEMORIA");
	config_kernel->PUERTO_MEMORIA 				= config_get_string_value(config, "PUERTO_MEMORIA");
	config_kernel->IP_FILESYSTEM 				= config_get_string_value(config, "IP_FILESYSTEM");
	config_kernel->PUERTO_FILESYSTEM 			= config_get_string_value(config, "PUERTO_FILESYSTEM");
	config_kernel->IP_CPU 						= config_get_string_value(config, "IP_CPU");
    config_kernel->PUERTO_CPU					= config_get_string_value(config, "PUERTO_CPU");
    config_kernel->PUERTO_ESCUCHA 				= config_get_string_value(config, "PUERTO_ESCUCHA");
    config_kernel->ALGORITMO_PLANIFICACION 		= config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    config_kernel->ESTIMACION_INICIAL 			= config_get_int_value(config, "ESTIMACION_INICIAL");
    config_kernel->HRRN_ALFA 					= config_get_int_value(config, "HRRN_ALFA");
    config_kernel->GRADO_MAX_MULTIPROGRAMACION 	= config_get_int_value(config, "GRADO_MAX_MULTIPROGRAMACION");

    // TODO Alejandro: Levantar la configuración de los recursos e instancias

    log_info(logger, "La configuración se cargó en 'config_kernel' ");
}

int conectar_con_cpu(){

	log_info(logger, "Iniciando conexión con CPU con la IP %s y PUERTO:%s ", config_kernel->IP_CPU, config_kernel->PUERTO_CPU);
    int conexion = crear_conexion(config_kernel->IP_CPU, config_kernel->PUERTO_CPU);

    if(conexion == -1){
    	log_info(logger, "No se pudo efectuar la conexión con CPU");
    }
    else{
        log_info(logger, "Se realizó exitosamente la conexión con CPU");
    }

    return conexion;
}

void conexion_con_memoria(char* ip,char* puerto,t_log* logger){
	int conexion_memoria = crear_conexion(ip,puerto);
	enviar_handshake(conexion_memoria,KERNEL);

	recibir_operacion(conexion_memoria);
	recibir_mensaje(conexion_memoria,logger);
}
