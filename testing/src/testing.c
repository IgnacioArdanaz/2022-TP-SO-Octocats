#include <stdio.h>
#include <stdlib.h>
#include <pcb.h>

int main(void) {
	PCB_t* pcb1 = pcb_create();
	pcb1->pid = 1;
	PCB_t* pcb2 = pcb_create();
	pcb2->pid = 2;
	t_list* lista = list_create();
	printf("%d", list_add(lista, pcb1));
	printf("%d", list_add(lista, pcb2));

	printf("Index de pcb 1: %d",pcb_find_index(lista,1));


	list_destroy_and_destroy_elements(lista,pcb_destroy);
}
