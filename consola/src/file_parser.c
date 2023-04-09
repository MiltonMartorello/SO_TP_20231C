#include "file_parser.h"

t_programa* parsear_programa(char * archivo, t_log * logger){

	if (access(archivo, F_OK) != 0) {
	    // El archivo no existe
	    printf("Error: El archivo %s no existe.\n", archivo);
	    return NULL;
	}

	// r => modo READ
	FILE *file = fopen(archivo, "r");

	if (file == NULL) {
		printf("No se puede abrir el archvivo. error:[%d]", errno);
		return NULL;
	}

	t_programa* programa = crear_programa();

	char* linea = NULL;
	size_t length = 0;

	bool error = false;
	while(getline(&linea, &length, file) != -1) {
		if (length > 0) {
			error = (parsear_instrucciones(linea, programa->instrucciones, logger) != 0);
			if (error) break;
		}
	}

	fclose(file);
	free(linea);

	if (error) {
		printf("error de parseo de archivo de pseudocodigo\n");
		programa_destroy(programa);
		return NULL;
	}
	else {
		return programa;
	}

}


t_programa* crear_programa(void){
	t_list* instrucciones = list_create();
	t_programa* programa = malloc(sizeof(t_programa));
	programa->size = sizeof(t_programa);
	programa->instrucciones = instrucciones;

	return programa;
}

void programa_destroy(t_programa* programa) {

	if (programa->instrucciones != NULL)
		list_destroy(programa->instrucciones);
	//TODO MALLOC DEL programa->size?
	free(programa);
}


int parsear_instrucciones(char* linea, t_list* instrucciones, t_log* logger){
	int resultado = EXIT_SUCCESS;
	t_instruccion* instruccion;
	linea = string_replace(linea, "\n", "");
	log_info(logger, "Parseando la linea %s", linea);
	char** parametros = string_split(linea, " ");
	char* funcion = parametros[0];
	log_info(logger, "Detectada la funcion %s", funcion);
	char* v;
	int i = 1;
	while ((v = parametros[i]) != NULL) {
		log_info(logger, "con parámetro nro %d = %s",i,v);
		i++;
	}
	if (i == 1){
		log_info(logger, "sin parámetros");
	}


	if (strcmp(funcion, "SET") == 0) {
		instruccion = crear_instruccion(ci_SET, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instruccion->parametros, strdup(parametros[2]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "I/O") == 0) {
		//TODO
	}
	else if (strcmp(funcion, "WAIT") == 0) {
		instruccion = crear_instruccion(ci_WAIT, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "SIGNAL") == 0) {
		instruccion = crear_instruccion(ci_SIGNAL, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "YIELD") == 0) {
		instruccion = crear_instruccion(ci_YIELD, true);
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "EXIT") == 0) {
		instruccion = crear_instruccion(ci_EXIT, true);
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "F_OPEN") == 0) {
		instruccion = crear_instruccion(ci_F_OPEN, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "F_READ") == 0) {
		instruccion = crear_instruccion(ci_F_READ, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instruccion->parametros, strdup(parametros[2]));
		list_add(instruccion->parametros, strdup(parametros[3]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "F_WRITE") == 0) {
		instruccion = crear_instruccion(ci_F_WRITE, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instruccion->parametros, strdup(parametros[2]));
		list_add(instruccion->parametros, strdup(parametros[3]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "F_TRUNCATE") == 0) {
		instruccion = crear_instruccion(ci_F_TRUNCATE, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instruccion->parametros, strdup(parametros[2]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "F_SEEK") == 0) {
		instruccion = crear_instruccion(ci_F_SEEK, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instruccion->parametros, strdup(parametros[2]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "F_CLOSE") == 0) {
		instruccion = crear_instruccion(ci_F_CLOSE, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "CREATE_SEGMENT") == 0) {
		instruccion = crear_instruccion(ci_CREATE_SEGMENT, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instruccion->parametros, strdup(parametros[2]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "DELETE_SEGMENT") == 0) {
		instruccion = crear_instruccion(ci_DELETE_SEGMENT, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "MOV_IN") == 0) {
		instruccion = crear_instruccion(ci_MOV_IN, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instruccion->parametros, strdup(parametros[2]));
		list_add(instrucciones, instruccion);
	}
	else if (strcmp(funcion, "MOV_OUT") == 0) {
		instruccion = crear_instruccion(ci_MOV_OUT, false);
		list_add(instruccion->parametros, strdup(parametros[1]));
		list_add(instruccion->parametros, strdup(parametros[2]));
		list_add(instrucciones, instruccion);
	}
	else {
		printf("Tipo de instruccion desconocida:[%s]\n", funcion);
		resultado = EXIT_FAILURE;
	}

	liberar_memoria_parseo(parametros, funcion);
	return resultado;
}

t_instruccion* crear_instruccion(t_codigo_instruccion codigo, bool empty) {
	t_instruccion* instruccion = malloc(sizeof(t_instruccion));
	instruccion->codigo = codigo;

	if (empty)
		instruccion->parametros = NULL;
	else
		instruccion->parametros = list_create();

	return instruccion;
}


void liberar_memoria_parseo(char **parametros, char *funcion) {
	char *v;
	int i = 0;
	while ((v = parametros[i]) != NULL) {
		free(v);
		i++;
	}
	free(parametros);
}
