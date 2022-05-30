#include <stdio.h>
#include <stdlib.h>
#include <pcb.h>

//reproduzco el segmentation fault que nos está tirando kernel

void funcion1(){
	PCB_t* pcb = pcb_create();
	pcb_destroy(pcb);
	printf("funcion1 no tira segmentation fault\n");
}

void funcion2(){
	PCB_t* pcb = pcb_create();
	t_list* inst = list_create();
	pcb->instrucciones = inst;
	pcb_destroy(pcb);
	printf("funcion2 no tira segmentation fault\n");
}

void funcion3(){
	PCB_t* pcb = pcb_create();
	t_list* inst = list_create();
	//el set destruye la lista anterior y linkea con la nueva
	pcb_set(pcb,0,0,inst,0,0,0.0);
	pcb_destroy(pcb);
	printf("funcion3 no tira segmentation fault\n");
}


int main(void) {
	funcion1(); //esta no da segmentation fault
	funcion2(); //esta no da segmentation fault
	funcion3(); //esta da segmentation fault
}
