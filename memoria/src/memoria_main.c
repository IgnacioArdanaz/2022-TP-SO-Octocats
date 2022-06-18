#include "memoria_main.h"

void testeando_boludeces(){
	int tam_memoria = 8000;
	int nro_marco = 34;
	int tam_pagina = 16;
	int desplaz = 15;
	void* memoria = malloc(tam_memoria);
	printf("INICIO DE LA MEMORIA: %p\n", memoria);
	printf("FIN DE LA MEMORIA: %p\n", memoria + tam_memoria);
	printf("INICIO DEL MARCO %d: %p\n",nro_marco, memoria + nro_marco * tam_pagina);
	int *ubicacion = memoria + nro_marco * tam_pagina + desplaz;
	printf("UBICACION DEL DATO en desplz %d: %p\n", desplaz, ubicacion);
	printf("DATO EN LA UBICACION ANTES DE ASIGNACION: %d\n", *ubicacion);
	int dato_a_asignar = 8931;
	*ubicacion = 8931;
	printf("DATO ASIGNADO: %d\n",dato_a_asignar);
	printf("DATO EN LA UBICACION LUEGO DE ASIGNACION: %d\n", *ubicacion);
	free(memoria);

	t_list* l = list_create();
	int a [] = {1,2,3,4};
	list_add(l,a);

	int* b = list_get(l,0);

	printf("%d", b[3]);


}

int main(void) {

//	testeando_boludeces();

	inicializar_memoria();

	cliente_cpu = esperar_cliente(logger, "MEMORIA", server_memoria);
	cliente_kernel = esperar_cliente(logger, "MEMORIA", server_memoria);

	while(true); //simula que sigue corriendo

	return 0;

}
