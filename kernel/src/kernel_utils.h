#ifndef KERNEL_UTILS_H_
#define KERNEL_UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include<semaphore.h>
#include<commons/collections/queue.h>
#include<commons/collections/list.h>
#include<commons/collections/dictionary.h>
#include "../../shared/src/sockets.h"
#include "../../shared/src/protocol.h"
#include "../../shared/src/structures.h"
#include "../../shared/src/pcb.h"

typedef struct{
	int cliente;
	char* server_name;
} thread_args;

typedef enum {
    FIFO,
	SRT,
} algoritmo_t;

void inicializar_kernel();
void cerrar_kernel();
int escuchar_servidor(char* name, int server_socket, t_log* logger);
void procesar_socket(thread_args* argumentos);
void pasaje_new_ready();
void fifo_ready_execute();
void srt_ready_execute();
void imprimir_lista_ready();
PCB_t* fifo();
PCB_t* sjf();
t_log* logger;
t_config* config;

#endif /* KERNEL_UTILS_H_ */
