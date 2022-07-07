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

PCB_t* pcb_create();

void pcb_set(PCB_t* pcb,
		uint16_t pid,
		uint16_t tamanio,
		t_list* instrucciones,
		uint32_t pc,
		uint32_t tabla_paginas,
		double est_rafaga);

int pcb_find_index(t_list* lista, uint16_t pid);

void pcb_destroy(PCB_t* pcb);

#endif
