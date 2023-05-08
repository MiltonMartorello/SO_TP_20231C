#ifndef SRC_ESTRUCTURAS_H_
#define SRC_ESTRUCTURAS_H_
#include <commons/collections/list.h>
#include <commons/temporal.h>
#include <commons/bitarray.h>
/*
 * GENERAL
 * */

typedef enum
{
	MENSAJE,
	PAQUETE,
	KERNEL,
	CPU,
	FILESYSTEM,
	CONSOLA,
	PROGRAMA,
	PROGRAMA_FINALIZADO
} op_code;

typedef enum{
	PROCESO_DESALOJADO_POR_YIELD,
	PROCESO_FINALIZADO
} cod_proceso;

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
	t_codigo_instruccion codigo;
	t_list* parametros;
} t_instruccion;


typedef struct {
	int size;
	t_list* instrucciones;
} t_programa;


/*
 * PLANIFICACION
 * */

// REGISTROS CPU
//typedef union {
//    char* AX;
//    char*  BX;
//    char*  CX;
//    char*  DX;
//    char*  EAX;
//    char*  EBX;
//    char*  ECX;
//    char*  EDX;
//    char* * RAX;
//    char* * RBX;
//    char* * RCX;
//    char* * RDX;
//} t_registro;

typedef union {
    char AX[4];
    char BX[4];
    char CX[4];
    char DX[4];
    char EAX[8];
    char EBX[8];
    char ECX[8];
    char EDX[8];
    char RAX[16];
    char RBX[16];
    char RCX[16];
    char RDX[16];
} t_registro;

// PCB
typedef struct {
	int pid;
	t_list* MOCK_instrucciones;
	int program_counter;
	t_registro* registros;
	//t_list MOCK_tabla_segmento;
	int estimado_rafaga;
	//t_temporal* tiempo_llegada;
	//t_list* MOCK_tabla_archivos_abiertos;
} t_pcb;

t_instruccion* crear_instruccion(t_codigo_instruccion, bool);
void buffer_destroy(t_buffer*);

#endif /* SRC_ESTRUCTURAS_H_ */
