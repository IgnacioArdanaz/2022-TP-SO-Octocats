#include "pcb.h"

PCB_t* pcb_create(){
	PCB_t* pcb = malloc(sizeof(PCB_t));
	pcb->instrucciones = list_create();
	return pcb;
}

void pcb_set(PCB_t* pcb,
		uint16_t pid,
		uint16_t tamanio,
		t_list* instrucciones,
		uint32_t pc,
		uint32_t tabla_paginas,
		double est_rafaga){
	pcb->pid = pid;
	pcb->tamanio = tamanio;
	pcb->pc = pc;
	//antes de cambiar de puntero, destruyo toda existencia de la anterior
	list_destroy_and_destroy_elements(pcb->instrucciones,free);
	pcb->instrucciones = instrucciones;
	pcb->tabla_paginas = tabla_paginas;
	pcb->est_rafaga = est_rafaga;
}

void pcb_destroy(PCB_t* pcb){
	list_destroy_and_destroy_elements(pcb->instrucciones,free);
	free(pcb);
}
