#ifndef CPU_MAIN_H_
#define CPU_MAIN_H_

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include<semaphore.h>
#include<sockets.h>
#include<protocol.h>
#include<structures.h>
#include<pcb.h>

#define IP "127.0.0.1"

void inicializar_cpu();
void apagar_cpu();
void interrupcion();
void calculo_estimacion(PCB_t *pcb);
op_code iniciar_ciclo_instruccion(PCB_t *pcb);
instruccion_t* fetch(t_list* instrucciones, uint32_t pc);
int decode(instruccion_t* instruccion_ejecutar );
int execute(instruccion_t* instruccion_ejecutar);
int check_interrupt();

#endif /* CPU_MAIN_H_ */
