#ifndef STRUCTURES_H_
#define STRUCTURES_H_

typedef struct {
	uint16_t pid;
	uint16_t tamanio;
	char** instrucciones;
	uint32_t pc;
	uint32_t tabla_paginas;
	double est_rafaga;
}PCB_t;


#endif
