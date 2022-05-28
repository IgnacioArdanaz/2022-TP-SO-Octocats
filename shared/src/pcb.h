#ifndef PCB_H_
#define PCB_H_

#include<stdlib.h>
#include<stdint.h>
#include<commons/collections/list.h>

typedef struct {
	uint16_t pid;
	uint16_t tamanio;
	t_list* instrucciones;
	uint32_t pc;
	uint32_t tabla_paginas;
	double est_rafaga;
}PCB_t;

PCB_t* pcb_create(uint16_t pid,
		uint16_t tamanio,
		t_list* instrucciones,
		uint32_t pc,
		uint32_t tabla_paginas,
		double est_rafaga);

void pcb_destroy(PCB_t* pcb);

#endif
