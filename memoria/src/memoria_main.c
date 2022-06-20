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

void testeando_tablas(){

	int n1 = crear_tablas(0,1024);
	int n2 = crear_tablas(1,1024);
	int n3 = crear_tablas(2,1024);
	int n4 = crear_tablas(3,1024);
	int n5 = crear_tablas(4,1);

}

int main(void) {

//	testeando_boludeces();
//
	inicializar_memoria();
//	pthread_t hilo_kernel;
//	pthread_create(&hilo_kernel, NULL, (void*) escuchar_kernel, NULL);
//	pthread_detach(hilo_kernel);
//
//
//	pthread_t hilo_cpu;
//	pthread_create(&hilo_cpu, NULL, (void*) escuchar_cpu, NULL);
//	pthread_detach(hilo_cpu);


	testeando_tablas();



//	while(true); //simula que sigue corriendo

	return 0;

}
