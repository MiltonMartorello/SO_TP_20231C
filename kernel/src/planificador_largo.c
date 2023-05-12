#include "../include/planificador_largo.h"

int planificador_largo_plazo(void* void_logger) {
	t_log *logger = (t_log*) void_logger;

	log_info(logger, "Inicializado Hilo Planificador de Largo Plazo");
	return 1;
}

int planificador_corto_plazo(void* void_logger) {
	t_log *logger = (t_log*) void_logger;


	log_info(logger, "Inicializado Hilo Planificador de Corto Plazo");
	return 1;
}
