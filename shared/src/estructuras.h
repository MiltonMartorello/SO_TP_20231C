#ifndef SRC_ESTRUCTURAS_H_
#define SRC_ESTRUCTURAS_H_
#include <commons/collections/list.h>
#include <commons/temporal.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include <commons/collections/queue.h>
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


typedef struct {
	int socket;
	t_log *log;
	pthread_mutex_t* mutex;
} t_args_hilo_cliente;

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
// TODO REFACTORIZAR LOS REGISTROS A 4-8-16 BYTES
typedef union {
    char* AX;
    char* BX;
    char* CX;
    char* DX;
    char* EAX;
    char* EBX;
    char* ECX;
    char* EDX;
    char* RAX;
    char* RBX;
    char* RCX;
    char* RDX;
} t_registro;

typedef enum{
	NEW,
	READY,
	EXEC,
	BLOCK,
	EXIT
} t_estado;

typedef struct {
	t_queue* cola_ready;
	t_queue* cola_new;
	t_queue* cola_exec;
	t_queue* cola_block;
	t_queue* cola_exit;
} t_colas;

// PCB
typedef struct {
	int pid;
	t_list* instrucciones;
	int program_counter;
	t_registro registros;
	int estimado_rafaga;
	t_temporal* tiempo_llegada;
	t_estado estado_actual;
	t_list* tabla_archivos_abiertos;
	t_list* tabla_segmento;
} t_pcb;

t_instruccion* crear_instruccion(t_codigo_instruccion, bool);
void buffer_destroy(t_buffer*);

#endif /* SRC_ESTRUCTURAS_H_ */
