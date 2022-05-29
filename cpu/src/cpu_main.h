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
#include "../../shared/src/sockets.c"
#include "../../shared/src/protocol.c"
#include "../../shared/src/structures.h"

#define IP "127.0.0.1"

typedef enum {
    PROGRAMA,
	PROCESO,
	INTERRUPTION,
	EXIT,
	CONTINUE,
	BLOCKED
} op_code;

void inicializar_cpu();
void apagar_cpu();
void interrupcion();
op_code iniciar_ciclo_instruccion(PCB_t *pcb);
instruccion_t* fetch(t_list* instrucciones, uint32_t pc);
int decode(instruccion_t* instruccion_ejecutar );
int execute(instruccion_t* instruccion_ejecutar);
int check_interrupt();

#endif /* CPU_MAIN_H_ */
