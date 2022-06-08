#ifndef MEMORIA_MAIN_H_
#define MEMORIA_MAIN_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/config.h>
#include <sockets.c>
#include <protocol.c>

#define IP "127.0.0.1"
#define PUERTO "4444"

#endif /* MEMORIA_MAIN_H_ */

t_log* logger;
t_config* config;

void inicializar_memoria();
void apagar_memoria();
