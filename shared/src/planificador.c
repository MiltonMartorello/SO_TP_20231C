#include "planificador.h"

void iniciar_colas_planificacion(void) {

	colas_planificacion = malloc(sizeof(t_colas));
	colas_planificacion->cola_block = queue_create();
	colas_planificacion->cola_exec = queue_create();
	colas_planificacion->cola_exit = queue_create();
	colas_planificacion->cola_new = queue_create();
	colas_planificacion->cola_ready = queue_create();
}

void destroy_colas_planificacion(void) {

	queue_destroy(colas_planificacion->cola_block);
	queue_destroy(colas_planificacion->cola_exec);
	queue_destroy(colas_planificacion->cola_exit);
	queue_destroy(colas_planificacion->cola_new);
	queue_destroy(colas_planificacion->cola_ready);
	free(colas_planificacion);
}
