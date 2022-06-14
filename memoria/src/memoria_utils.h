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

t_log* logger;
t_config* config;

void apagar_memoria();
void inicializar_memoria();

#endif /* MEMORIA_UTILS_H_ */
