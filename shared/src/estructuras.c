#include <errno.h>
#include "shared.h"
#include "estructuras.h"

t_instruccion* crear_instruccion(t_codigo_instruccion codigo, bool empty) {
	t_instruccion* instruccion = malloc(sizeof(t_instruccion));
	instruccion->codigo = codigo;

	if (empty)
		instruccion->parametros = NULL;
	else
		instruccion->parametros = list_create();

	return instruccion;
}


void buffer_destroy(t_buffer* buffer) {
	free(buffer->stream);
	free(buffer);
}
