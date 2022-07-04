#ifndef CPU_MAIN_H_
#define CPU_MAIN_H_

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include <math.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include<semaphore.h>
#include<sockets.h>
#include<protocol.h>
#include<structures.h>
#include<pcb.h>
#include<string.h>

typedef struct{
	int32_t pagina;
	int32_t marco;
	clock_t ultima_referencia;
}TLB_t;

typedef struct{
	int32_t marco;
	int32_t desplazamiento;
}marco_t;

void inicializar_cpu();
void apagar_cpu();
void interrupcion();
void calculo_estimacion(PCB_t *pcb, op_code estado);
op_code iniciar_ciclo_instruccion(PCB_t *pcb);
instruccion_t* fetch(t_list* instrucciones, uint32_t pc);
int decode(instruccion_t* instruccion_ejecutar );
int execute(instruccion_t* instruccion_ejecutar,uint32_t tabla_paginas);
int check_interrupt();
marco_t traducir_direccion(uint32_t dir_logica,uint32_t tabla_paginas);
void ejecutarWrite(uint32_t dir_logica,uint32_t valor,uint32_t tabla_paginas );
int ejecutarRead(uint32_t dir_logica,uint32_t tabla_paginas);
void reemplazo_tlb_FIFO(uint32_t numero_pagina, int32_t marco );
void inicializar_tlb();
TLB_t *crear_entrada_tlb(int32_t pagina, int32_t marco);
void freemplazo_tlb(uint32_t numero_pagina, int32_t marco);
void reemplazo_tlb_LRU(uint32_t numero_pagina, int32_t marco );
bool marco_en_tlb(int32_t marco,int32_t pagina);
void limpiar_tlb();
#endif /* CPU_MAIN_H_ */
