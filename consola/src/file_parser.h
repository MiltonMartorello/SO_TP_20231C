#ifndef FILE_PARSER_H_
#define FILE_PARSER_H_

#include <errno.h>
#include <shared.h>

t_programa* parsear_programa(char *, t_log*);
t_programa* crear_programa(void);
void programa_destroy(t_programa*);
int parsear_instrucciones(char*, t_list*, t_log*);
void liberar_memoria_parseo(char **, char *);
#endif /* FILE_PARSER_H_ */
