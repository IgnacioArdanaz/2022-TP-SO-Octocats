#ifndef KERNEL_UTILS_H_
#define KERNEL_UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>

#include "../../shared/src/sockets.h"
#include "../../shared/src/protocol.h"
#include "../../shared/src/structures.h"

typedef struct{
	int cliente;
	char* server_name;
} thread_args;

t_log* logger;
t_config* config;
void inicializar();
void procesar_socket(thread_args* argumentos);
int escuchar_servidor(char* name, int server_socket);

#endif /* KERNEL_UTILS_H_ */
