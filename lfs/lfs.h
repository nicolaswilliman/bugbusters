#ifndef LFS_H_
#define LFS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <nuestro_lib/nuestro_lib.h>

t_log* logger_LFS;
t_config* config;

void recibirConexionMemoria(void);

#endif /* LFS_H_ */
