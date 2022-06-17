#ifndef MEMORIA_UTILS_H_
#define MEMORIA_UTILS_H_

#include<stdio.h>
#include<stdlib.h>
//#include<stdbool.h>
//#include<pthread.h>
#include<commons/log.h>
#include<commons/config.h>
//#include<commons/string.h>
//#include<readline/readline.h>
//#include<semaphore.h>
//#include<commons/collections/queue.h>
#include<commons/collections/list.h>
//#include<commons/collections/dictionary.h>
#include <sockets.h>
#include <protocol.h>
//#include <structures.h>
//#include <pcb.h>

typedef unsigned int* fila_1er_nivel;

typedef struct {
	unsigned int* direccion;
	unsigned char modificado;
	unsigned char presencia;
} fila_2do_nivel;

t_log* logger;
t_config* config;

void apagar_memoria();
void inicializar_memoria();
uint32_t crear_tabla_1er_nivel();
void crear_tablas_2do_nivel();
void asignar_marcos(t_list* tabla_2do_nivel);

#endif /* MEMORIA_UTILS_H_ */
