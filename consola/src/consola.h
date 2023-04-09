#ifndef CONSOLA_H_
#define CONSOLA_H_

#include <stdio.h>
#include <stdlib.h>
#include <shared.h>
#include <commons/config.h>

#define PATH_CONFIG "consola.config"
#define PATH_LOG "consola.log"

void terminar_programa(int, t_log*, t_config*);

#endif /* CONSOLA_H_ */
