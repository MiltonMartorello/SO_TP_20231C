#ifndef I_CPU_H_
#define I_CPU_H_

#include "planificador_corto.h"

extern int socket_cpu;
extern t_colas* colas_planificacion;
extern sem_t cpu_liberada;
extern sem_t proceso_enviado;

extern t_list* lista_recursos;

void manejar_respuesta_cpu(void* args_hilo);

void pasar_a_ready_segun_algoritmo(char* algoritmo,t_pcb* proceso,t_log* logger);
char * recibir_string(void);

void actualizar_pcb(t_pcb* pcb, t_contexto_proceso* contexto);
void procesar_contexto(t_pcb* pcb, op_code cod_op, char* algoritmo, t_log* logger);

void bloqueo_io(void* vArgs);
void procesar_wait_recurso(char* nombre,t_pcb* pcb,char* algoritmo,t_log* logger);
void procesar_signal_recurso(char* nombre,t_pcb* pcb,char* algoritmo,t_log* logger);
void procesar_f_open(t_pcb* pcb);
void procesar_f_close(t_pcb* pcb);
void procesar_f_seek(t_pcb* pcb);
void procesar_f_read(t_pcb* pcb);
void procesar_f_write(t_pcb* pcb);
void procesar_f_truncate(t_pcb* pcb);
#endif