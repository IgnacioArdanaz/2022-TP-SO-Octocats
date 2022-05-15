#ifndef KERNEL_UTILS_H_
#define KERNEL_UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<commons/collections/queue.h>
#include<readline/readline.h>

#include "../../shared/src/sockets.h"
#include "../../shared/src/protocol.h"
#include "../../shared/src/structures.h"

typedef struct{
	int cliente;
	char* server_name;
} procesar_consola_args;

t_config* config;
t_log* logger;

void inicializar_estructuras();
void procesar_consola(procesar_consola_args* argumentos);
int escuchar_servidor(char* name, int server_socket);
void pasaje_new_ready();
PCB_t* sjf();
PCB_t* fifo();
#endif /* KERNEL_UTILS_H_ */
