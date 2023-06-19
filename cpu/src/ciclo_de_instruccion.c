#include "../include/ciclo_de_instruccion.h"

extern t_contexto_proceso* proceso;
extern t_cpu_config* cpu_config;
extern t_log* cpu_logger;
extern int socket_kernel;
extern int socket_memoria;
extern t_reg registros_cpu;

void ciclo_de_instruccion(t_contexto_proceso* proceso,int socket){

	bool fin_de_ciclo = false;
	t_instruccion* una_instruccion;
	t_list* parametros;
	while(!fin_de_ciclo){

		una_instruccion = list_get(proceso->instrucciones, proceso->program_counter);
		//log_info(cpu_logger, "Cód instrucción: %d", una_instruccion->codigo);
		parametros = una_instruccion->parametros;
		proceso->program_counter++;

		switch (una_instruccion->codigo)
		{
		case ci_SET:
			execute_set(list_get(parametros,0),list_get(parametros,1));
			break;
		case ci_MOV_IN:
			execute_mov_in(list_get(parametros,0),atoi(list_get(parametros,1)));
			break;
		case ci_MOV_OUT:
			execute_mov_out(atoi(list_get(parametros,0)),list_get(parametros,1));
			break;
		case ci_IO: //TODO funcion para loguear instrucciones
			execute_io(atoi(list_get(parametros,0)));
			fin_de_ciclo = true;
			break;
		case ci_F_OPEN:
			execute_f_open(list_get(parametros,0));
			fin_de_ciclo = true;
			break;
		case ci_F_CLOSE:
			execute_f_close(list_get(parametros,0));
			fin_de_ciclo = true;
			break;
		case ci_F_SEEK:
			execute_f_seek(list_get(parametros,0),atoi(list_get(parametros,1)));
			fin_de_ciclo = true;
			break;
		case ci_F_READ:
			execute_f_read(list_get(parametros,0),atoi(list_get(parametros,1)),atoi(list_get(parametros,2)));
			fin_de_ciclo = true;
			break;
		case ci_F_WRITE:
			execute_f_write(list_get(parametros,0),atoi(list_get(parametros,1)),atoi(list_get(parametros,2)));
			fin_de_ciclo = true;
			break;
		case ci_F_TRUNCATE:
			execute_f_truncate(list_get(parametros,0),atoi(list_get(parametros,1)));
			fin_de_ciclo = true;
			break;
		case ci_WAIT:
			execute_wait(list_get(parametros,0));
			fin_de_ciclo = true;
			break;
		case ci_SIGNAL:
			execute_signal(list_get(parametros,0));
			fin_de_ciclo = true;
			break;
		case ci_CREATE_SEGMENT:
			execute_create_segment(atoi(list_get(parametros, 0)),atoi(list_get(parametros, 1)));
			fin_de_ciclo = true;
			break;
		case ci_DELETE_SEGMENT:
			execute_delete_segment(atoi(list_get(parametros, 0)));
			fin_de_ciclo = true;
			break;
		case ci_YIELD:
			execute_yield();
			fin_de_ciclo = true;
			break;
		case ci_EXIT:
			execute_exit();
			fin_de_ciclo = true;
			break;
		default:
			log_info(cpu_logger, "Instruccion desconocida.");
			break;
		}

	}
	//free(una_instruccion);
}

void execute_set(char* registro, char* valor) {
	log_info(cpu_logger,"PID: <%d> - Ejecutando: <SET> - <%s> - <%s>",proceso->pid, registro, valor);
	usleep(cpu_config->retardo_instruccion * 1000);
	set_valor_registro(registro,valor);
}

void execute_mov_in(char* registro,int direccion_logica) {
	log_info(cpu_logger,"PID: <%d> - Ejecutando: <MOV_IN> - <%s> - <%d>",proceso->pid, registro, direccion_logica);
	int direccion_fisica = traducir_a_direccion_fisica(direccion_logica, proceso, 0);
	t_segmento* segmento = obtener_segmento(direccion_logica, proceso->tabla_segmentos);

	int valor = leer_memoria(direccion_fisica);
	log_info(cpu_logger, "PID: <%d> - Acción: <LEER> - Segmento: <%d> - Dirección Física: <%d> - Valor: <%d>",proceso->pid,segmento->segmento_id,direccion_fisica,valor);

	set_valor_registro(registro, valor);
}

void execute_mov_out(int direccion_logica, char* registro) {
	log_info(cpu_logger,"PID: <%d> - Ejecutando: <MOV_OUT> - <%d> - <%s>",proceso->pid,direccion_logica,registro);
	char* valor_registro = get_valor_registro(registro);
	int direccion_fisica = traducir_a_direccion_fisica(direccion_logica, proceso, 0);
	t_segmento* segmento = obtener_segmento(direccion_logica, proceso->tabla_segmentos);

	escribir_memoria(direccion_fisica, valor_registro);

	//	char* respuesta = recibir_string(socket_memoria);
	//	if(string_equals_ignore_case(respuesta, "OK")){
	//		log_info(cpu_logger, "PID: <%d> - Acción: <ESCRIBIR> - Segmento: <%d> - Dirección Física: <%d> - Valor: <%d>",proceso->pid,segmento->id,direccion_fisica,valor);
	//	}
}

void execute_io(int tiempo) {
	log_info(cpu_logger,"PID: <%d> - Ejecutando: <IO> - <%d>",proceso->pid, tiempo);
	devolver_proceso(proceso,PROCESO_BLOQUEADO,cpu_logger);
	enviar_entero(socket_kernel, tiempo);
	log_info(cpu_logger,"Se devolvió el proceso a KERNEL con el codigo PROCESO_BLOQUEADO");
}

void execute_f_open(char* nombre_archivo) {
	log_info(cpu_logger,"PID: <%d> - Ejecutando: <F_OPEN> - <%s>", proceso->pid, nombre_archivo);
	devolver_proceso(proceso, PROCESO_DESALOJADO_POR_F_OPEN, cpu_logger);
	enviar_mensaje(nombre_archivo, socket_kernel, cpu_logger);
}

void execute_f_close(char* nombre_archivo) {
	log_info(cpu_logger,"PID: <%d> - Ejecutando: <F_CLOSE> - <%s>", proceso->pid, nombre_archivo);
	devolver_proceso(proceso, PROCESO_DESALOJADO_POR_F_CLOSE, cpu_logger);
	enviar_mensaje(nombre_archivo, socket_kernel, cpu_logger);
}

void execute_f_seek(char* nombre_archivo, int posicion) {
	log_info(cpu_logger,"PID: <%d> - Ejecutando: <F_SEEK> - <%s> - <%d>", proceso->pid, nombre_archivo, posicion);
	devolver_proceso(proceso, PROCESO_DESALOJADO_POR_F_SEEK, cpu_logger);
	enviar_mensaje(nombre_archivo, socket_kernel, cpu_logger);
	enviar_entero(socket_kernel, posicion);
}

void execute_f_read(char* nombre_archivo, int direccion_logica, int cant_bytes) {
	int direccion_fisica = traducir_a_direccion_fisica(direccion_logica, proceso->tabla_segmentos,cant_bytes);
	if(direccion_fisica != -1){
		log_info(cpu_logger,"PID: <%d> - Ejecutando: <F_READ> - <%s> - <%d> - <%d>", proceso->pid, nombre_archivo, direccion_logica, cant_bytes);
		devolver_proceso(proceso, PROCESO_DESALOJADO_POR_F_READ, cpu_logger);
		enviar_mensaje(nombre_archivo, socket_kernel, cpu_logger);
		enviar_entero(socket_kernel, direccion_fisica);
		enviar_entero(socket_kernel, cant_bytes);
	}
	else{
		devolver_proceso(proceso, SEG_FAULT, cpu_logger); //TODO : usar este enum? o crear otro?
	}
}

void execute_f_write(char* nombre_archivo, int direccion_logica, int cant_bytes) {
	int direccion_fisica = traducir_a_direccion_fisica(direccion_logica, proceso->tabla_segmentos,cant_bytes);

	if(direccion_fisica != -1){
		log_info(cpu_logger,"PID: <%d> - Ejecutando: <F_WRITE> - <%s> - <%d> - <%d>", proceso->pid, nombre_archivo, direccion_logica, cant_bytes);
		devolver_proceso(proceso, PROCESO_DESALOJADO_POR_F_WRITE, cpu_logger);
		enviar_mensaje(nombre_archivo, socket_kernel, cpu_logger);
		enviar_entero(socket_kernel, direccion_fisica);
		enviar_entero(socket_kernel, cant_bytes);

	}
	else{
		devolver_proceso(proceso, PROCESO_DESALOJADO_POR_SEG_FAULT, cpu_logger);
	}
}

void execute_f_truncate(char* nombre_archivo, int tamanio) {
	log_info(cpu_logger,"PID: <%d> - Ejecutando: <F_TRUNCATE> - <%s> - <%d>",proceso->pid,nombre_archivo,tamanio);
	devolver_proceso(proceso, PROCESO_DESALOJADO_POR_F_TRUNCATE,cpu_logger);
	enviar_mensaje(nombre_archivo, socket_kernel, cpu_logger);
	enviar_entero(socket_kernel, tamanio);
}

void execute_wait(char* recurso) {
	log_info(cpu_logger,"PID: <%d> - Ejecutando: <WAIT> - <%s>", proceso->pid, recurso);
	devolver_proceso(proceso, PROCESO_DESALOJADO_POR_WAIT, cpu_logger);
	enviar_mensaje(recurso, socket_kernel, cpu_logger);

	log_info(cpu_logger,"Se devolvió el proceso a KERNEL con el codigo PROCESO_DESALOJADO_POR_WAIT");
}

void execute_signal(char* recurso) {
	log_info(cpu_logger,"PID: <%d> - Ejecutando: <SIGNAL> - <%s>", proceso->pid, recurso);
	devolver_proceso(proceso, PROCESO_DESALOJADO_POR_SIGNAL, cpu_logger);
	enviar_mensaje(recurso, socket_kernel, cpu_logger);

	log_info(cpu_logger,"Se devolvió el proceso a KERNEL con el codigo PROCESO_DESALOJADO_POR_SIGNAL");
}

void execute_create_segment(int id_segmento, int tamanio) {
	log_info(cpu_logger,"PID: <%d> - Ejecutando: <CREATE_SEGMENT> - <%d> - <%d>", proceso->pid, id_segmento, tamanio);
	devolver_proceso(proceso, PROCESO_DESALOJADO_POR_CREATE_SEGMENT, cpu_logger);
	enviar_entero(socket_kernel, id_segmento);
	enviar_entero(socket_kernel, tamanio);
}

void execute_delete_segment(int id_segmento) {
	log_info(cpu_logger,"PID: <%d> - Ejecutando: <DELETE_SEGMENT> - <%d>",proceso->pid, id_segmento);
	devolver_proceso(proceso, PROCESO_DESALOJADO_POR_DELETE_SEGMENT, cpu_logger);
	enviar_entero(socket_kernel, id_segmento);
}

void execute_yield(void) {
	log_info(cpu_logger,"PID: <%d> - Ejecutando: <YIELD>",proceso->pid);
	devolver_proceso(proceso,PROCESO_DESALOJADO_POR_YIELD,cpu_logger);
	log_info(cpu_logger,"Se devolvió el proceso a KERNEL con el codigo PROCESO_DESALOJADO_POR_YIELD");
}

void execute_exit(void) {
	log_info(cpu_logger,"PID: <%d> - Ejecutando: <EXIT>",proceso->pid);
	devolver_proceso(proceso,PROCESO_FINALIZADO,cpu_logger);
	log_info(cpu_logger,"Se devolvió el proceso a KERNEL con el codigo PROCESO_FINALIZADO");
}

//--------------MMU
int traducir_a_direccion_fisica(int direccion_logica , t_contexto_proceso* proceso,int cant_bytes) {

	int desplazamiento_segmento = direccion_logica%cpu_config->tam_max_segmento;
	t_segmento* segmento = obtener_segmento(direccion_logica,proceso->tabla_segmentos);

	int offset = desplazamiento_segmento + cant_bytes;
	if (offset > segmento->tam_segmento){ //SEG_FAULT
		log_info(cpu_logger,"PID: <%d> - Error SEG_FAULT- Segmento: <%d> - Offset: <%d> - Tamaño: <%d>",proceso->pid,segmento->segmento_id,offset,segmento->tam_segmento);
		return -1;
	}
	return segmento->inicio + desplazamiento_segmento;
}

t_segmento* obtener_segmento(int direccion_logica,t_list* tabla_segmentos){
	int num_segmento = floor(direccion_logica/cpu_config->tam_max_segmento);
	return (t_segmento*) list_get(tabla_segmentos, num_segmento);
}

//--------------LLAMADAS A MEMORIA
int leer_memoria(int direccion_fisica){
	enviar_entero(socket_memoria, LEER_DIRECCION);
	enviar_entero(socket_memoria, direccion_fisica);
	char* respuesta = recibir_string(socket_memoria);
	return respuesta;
}

void escribir_memoria(int direccion_fisica, char* valor_a_escribir){
	enviar_entero(socket_memoria, ESCRIBIR_DIRECCION);
	enviar_entero(socket_memoria, direccion_fisica);
	enviar_mensaje(valor_a_escribir, socket_memoria, cpu_logger);
}

//-------------MANEJO DE REGISTROS
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

char* get_valor_registro(char* nombre_registro){
	char tipo_registro = nombre_registro[0];
	int posicion = posicion_registro(nombre_registro);

	switch (tipo_registro)
	{
	case 'R':
		return registros_cpu.registros_16[posicion];
		break;
	case 'E':
		return registros_cpu.registros_8[posicion];
		break;
	default:
		return registros_cpu.registros_4[posicion];
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

//--------------------
void devolver_proceso(t_contexto_proceso* proceso,int codigo,t_log* logger){
	actualizar_registros_pcb(&proceso->registros);
	enviar_contexto(socket_kernel,proceso,codigo,logger);
}
