#include "pcb.h"

PCB_t* pcb_create(uint16_t pid,
		uint16_t tamanio,
		t_list* instrucciones,
		uint32_t pc,
		uint32_t tabla_paginas,
		double est_rafaga){
	PCB_t* proceso = malloc(sizeof(PCB_t));
	proceso->instrucciones = list_create();
	proceso->pid = pid;
	proceso->tamanio = tamanio;
	proceso->pc = pc;
	proceso->instrucciones = instrucciones;
	proceso->tabla_paginas = tabla_paginas;
	proceso->est_rafaga = est_rafaga;
	return proceso;
}

void pcb_destroy(PCB_t* pcb){
	list_destroy_and_destroy_elements(pcb->instrucciones,free);
	free(pcb);
}
