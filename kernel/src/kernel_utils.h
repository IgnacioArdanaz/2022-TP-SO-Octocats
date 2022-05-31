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
#include <sockets.h>
#include <protocol.h>
#include <structures.h>
#include <pcb.h>

typedef struct{
	int cliente;
	char* server_name;
} thread_args;

t_log* logger;
t_config* config;

void inicializar_kernel();
int escuchar_servidor(char* name, int server_socket);
void procesar_socket(thread_args* argumentos);
void pasaje_new_ready();
void fifo_ready_execute();
void srt_ready_execute();
void loggear_estado_de_colas();
void imprimir_lista_ready();
void esperar_cpu();
void ejecutar_io();
//PCB_t* fifo();
//PCB_t* sjf();

#endif /* KERNEL_UTILS_H_ */
