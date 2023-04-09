#ifndef SRC_ESTRUCTURAS_H_
#define SRC_ESTRUCTURAS_H_
#include <commons/collections/list.h>

/*
 * GENERAL
 * */

typedef enum
{
	MENSAJE,
	PAQUETE
}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;


/*
 * PROGRAMA E INSTRUCCIONES
 * CONSOLA - KERNEL
 * */

typedef enum {
	ci_SET = 1,
	ci_WAIT,
	ci_SIGNAL,
	ci_YIELD,
	ci_IO,
	ci_F_OPEN,
	ci_F_READ,
	ci_F_WRITE,
	ci_F_TRUNCATE,
	ci_F_SEEK,
	ci_F_CLOSE,
	ci_CREATE_SEGMENT,
	ci_DELETE_SEGMENT,
	ci_MOV_IN,
	ci_MOV_OUT,
	ci_EXIT,
} t_codigo_instruccion;


typedef struct {
	t_codigo_instruccion	codigo;
	t_list*					parametros;
} t_instruccion;


typedef struct {
	int			size;
	t_list*		instrucciones;
} t_programa;


t_instruccion* crear_instruccion(t_codigo_instruccion, bool);


#endif /* SRC_ESTRUCTURAS_H_ */
