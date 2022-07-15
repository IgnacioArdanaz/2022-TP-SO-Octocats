#ifndef KERNEL_UTILS_H_
#define KERNEL_UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
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
void pasaje_a_ready();
void solicitar_tabla_paginas(PCB_t* proceso);
void fifo_ready_execute();
void srt_ready_execute();
void loggear_estado_de_colas();
void esperar_cpu();
void ejecutar_io();
void suspendiendo(PCB_t* pcb);
bool esta_suspendido(uint16_t pid);
PCB_t* seleccionar_proceso_srt();
void desalojar_cpu();
void printear_estado_semaforos();

#endif /* KERNEL_UTILS_H_ */
