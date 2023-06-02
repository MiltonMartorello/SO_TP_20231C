#include "../Include/cpu.h"

t_log* cpu_logger;
t_config* config;
int socket_cpu;
int socket_kernel;
int socket_memoria;
t_cpu_config* cpu_config;
t_reg registros_cpu;

char* IP_CPU = "127.0.0.1";

int main(int argc, char **argv) {

	if(argc < 2){
		printf("Falta path a archivo de configuración.\n");
		return EXIT_FAILURE;
	}

	char* config_path = argv[1];

	cpu_logger = iniciar_logger("cpu.log");
	cargar_config(config_path);

	//conexion_a_memoria(cpu_config->ip_memoria,cpu_config->puerto_memoria,cpu_logger);

	correr_servidor();

	terminar();
	return EXIT_SUCCESS;
}

void terminar(void)
{
	if(cpu_logger != NULL) {
		log_destroy(cpu_logger);
	}

	if(config != NULL) {
		config_destroy(config);
	}

	liberar_conexion(socket_kernel);
	liberar_conexion(socket_memoria);
	liberar_conexion(socket_cpu);
	free(cpu_config);
}

void cargar_config(char* path){
	config = iniciar_config(path);
	cpu_config = malloc(sizeof(t_cpu_config));

	cpu_config->ip_memoria = config_get_string_value(config,"IP_MEMORIA");
	cpu_config->puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA");
	cpu_config->puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");
	cpu_config->retardo_instruccion = config_get_int_value(config,"RETARDO_INSTRUCCION");
	cpu_config->tam_max_segmento = config_get_int_value(config,"TAM_MAX_SEGMENTO");

}


void conexion_a_memoria(char* ip,char* puerto,t_log* logger){
	socket_memoria = crear_conexion(ip,puerto);
	enviar_handshake(socket_memoria,CPU);
	log_info(logger,"El módulo CPU se conectará con el ip %s y puerto: %s  ",ip,puerto);

}

void correr_servidor(void){

	socket_cpu = iniciar_servidor(cpu_config->puerto_escucha);

	//TODO verificar socket != -1
	log_info(cpu_logger, "Iniciada la conexión de servidor de cpu: %d",socket_cpu);

	socket_kernel = esperar_cliente(socket_cpu, cpu_logger);
	//TODO verificar socket != -1

	int modulo = recibir_operacion(socket_kernel);

	switch(modulo){
		case KERNEL:
			log_info(cpu_logger, "Kernel Conectado.");
			while(1){
				recibir_operacion(socket_kernel);//pcb
				t_contexto_proceso* proceso = recibir_contexto(socket_kernel, cpu_logger);

				ciclo_de_instruccion(proceso,socket_kernel);
				log_info(cpu_logger,"Se ejecuto un proceso");
				//liberar_pcb(pcb);
			}
			break;
		case -1:
			log_error(cpu_logger, "Se desconectó el cliente.");
			close(socket_kernel);
			exit(EXIT_FAILURE);
		default:
			log_error(cpu_logger, "Codigo de operacion desconocido.");
			return;
			break;

	}
}


void ciclo_de_instruccion(t_contexto_proceso* proceso,int socket){

	bool fin_de_ciclo = false;
	t_instruccion* una_instruccion = malloc(sizeof(t_instruccion));

	while(!fin_de_ciclo){

		una_instruccion = list_get(proceso->instrucciones, proceso->program_counter);
		proceso->program_counter++;
		switch (una_instruccion->codigo)
		{
		case ci_SET:
			log_info(cpu_logger,"PID: <%d> - Ejecutando: <SET> - <%s> - <%s>",proceso->pid,(char*)list_get(una_instruccion->parametros,0),(char*)list_get(una_instruccion->parametros,1));
			usleep(cpu_config->retardo_instruccion * 1000);
			set_valor_registro((char *)list_get(una_instruccion->parametros,0),(char*)list_get(una_instruccion->parametros,1));
			break;

		case ci_YIELD:
			log_info(cpu_logger,"PID: <%d> - Ejecutando: <YIELD>",proceso->pid);
			devolver_proceso(socket,proceso,PROCESO_DESALOJADO_POR_YIELD,cpu_logger);
			log_info(cpu_logger,"Se devolvió el proceso a KERNEL con el codigo PROCESO_DESALOJADO_POR_YIELD");
			return;
			break;

		case ci_EXIT:
			log_info(cpu_logger,"PID: <%d> - Ejecutando: <EXIT>",proceso->pid);
			devolver_proceso(socket,proceso,PROCESO_FINALIZADO,cpu_logger);
			log_info(cpu_logger,"Se devolvió el proceso a KERNEL con el codigo PROCESO_FINALIZADO");
			fin_de_ciclo = true;
			return;
			break;

		default:
			break;
		}

	}

	free(una_instruccion);
}

void devolver_proceso(int socket,t_contexto_proceso* proceso,int codigo,t_log* logger){
	actualizar_registros_pcb(&proceso->registros);
	enviar_contexto(socket,proceso,codigo,logger);
}


void loggear_registros(t_registro* registro) {
    log_info(cpu_logger, "Valores del registro:");
    log_info(cpu_logger, "AX: %s", registro->AX);
    log_info(cpu_logger, "BX: %s", registro->BX);
    log_info(cpu_logger, "CX: %s", registro->CX);
    log_info(cpu_logger, "DX: %s", registro->DX);
    log_info(cpu_logger, "EAX: %s", registro->EAX);
    log_info(cpu_logger, "EBX: %s", registro->EBX);
    log_info(cpu_logger, "ECX: %s", registro->ECX);
    log_info(cpu_logger, "EDX: %s", registro->EDX);
    log_info(cpu_logger, "RAX: %s", registro->RAX);
    log_info(cpu_logger, "RBX: %s", registro->RBX);
    log_info(cpu_logger, "RCX: %s", registro->RCX);
    log_info(cpu_logger, "RDX: %s", registro->RDX);
}


void set_valor_registro(char* nombre_registro, char* valor){

	char tipo_registro = nombre_registro[0];
	int posicion = posicion_registro(nombre_registro);
//	log_info(cpu_logger, "Set de tipo %c %s", tipo_registro, valor);
	switch (tipo_registro)
	{
	case 'R':
		strncpy(registros_cpu.registros_16[posicion],valor,16);
//		log_info(cpu_logger, "Actualizando registro R: %s", valor);
		break;

	case 'E':
		strncpy(registros_cpu.registros_8[posicion],valor,8);
//		log_info(cpu_logger, "Actualizando registro E: %s", valor);
		break;

	default:
		strncpy(registros_cpu.registros_4[posicion],valor,4);
//		log_info(cpu_logger, "Actualizando registro X: %s", valor);
		break;
	}
}

int posicion_registro(char* nombre_registro){

    int l = strlen(nombre_registro);
    char tipo[2];
    strcpy(tipo,&nombre_registro[l-2]);

	if(strcmp(tipo,"AX") == 0) return 0;
	if(strcmp(tipo,"BX") == 0) return 1;
	if(strcmp(tipo,"CX") == 0) return 2;
	if(strcmp(tipo,"DX") == 0) return 3;
	return NULL;
}


void actualizar_registros_pcb(t_registro* registros) {
    strcpy(registros->AX, registros_cpu.registros_4[0]);
    //log_info(cpu_logger, "Registro AX %s, %s", registros->AX, registros_cpu.registros_4[0]);

    strcpy(registros->BX, registros_cpu.registros_4[1]);
//    log_info(cpu_logger, "Registro BX: %.4s, %s", registros->BX, registros_cpu.registros_4[1]);

    strncpy(registros->CX, registros_cpu.registros_4[2], 5);
//    log_info(cpu_logger, "Registro CX: %.4s, %s", registros->CX, registros_cpu.registros_4[2]);

    strncpy(registros->DX, registros_cpu.registros_4[3], 5);
//    log_info(cpu_logger, "Registro DX: %.4s, %s", registros->DX, registros_cpu.registros_4[3]);

    strncpy(registros->EAX, registros_cpu.registros_8[0], 9);
//    log_info(cpu_logger, "Registro EAX: %.8s, %s", registros->EAX, registros_cpu.registros_8[0]);

    strncpy(registros->EBX, registros_cpu.registros_8[1], 9);
//    log_info(cpu_logger, "Registro EBX: %.8s, %s", registros->EBX, registros_cpu.registros_8[1]);

    strncpy(registros->ECX, registros_cpu.registros_8[2], 9);
//    log_info(cpu_logger, "Registro ECX: %.8s, %s", registros->ECX, registros_cpu.registros_8[2]);

    strncpy(registros->EDX, registros_cpu.registros_8[3], 9);
//    log_info(cpu_logger, "Registro EDX: %.8s, %s", registros->EDX, registros_cpu.registros_8[3]);

    strncpy(registros->RAX, registros_cpu.registros_16[0], 17);
//    log_info(cpu_logger, "Registro RAX: %.16s, %s", registros->RAX, registros_cpu.registros_16[0]);

    strncpy(registros->RBX, registros_cpu.registros_16[1], 17);
//    log_info(cpu_logger, "Registro RBX: %.16s, %s", registros->RBX, registros_cpu.registros_16[1]);

    strncpy(registros->RCX, registros_cpu.registros_16[2], 17);
//    log_info(cpu_logger, "Registro RCX: %.16s, %s", registros->RCX, registros_cpu.registros_16[2]);

    strncpy(registros->RDX, registros_cpu.registros_16[3], 17);
//    log_info(cpu_logger, "Registro RDX: %.16s, %s", registros->RDX, registros_cpu.registros_16[3]);

    loggear_registros(registros);
}


