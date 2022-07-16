#include "memoria_main.h"

void testeando_boludeces(){
	/////// ESTA FUNCIONANDO EL SWAPEO DE ESTA MANERA
//	int fd = crear_archivo_swap(1, 64);
//	void* marco = malloc(64);
//	int a = 255;
//	memcpy(marco + 8, &a, 4);
//	free(marco);
//	int* puntual1 = marco + 8;
//	printf("Antes de bajarlo a swap: %d\n", *puntual1);
//	actualizar_marco_en_swap(fd, 0, marco, 64);
//	void* otro_marco = leer_marco_en_swap(fd, 0, 64);
//	int* puntual = otro_marco + 8;
//	printf("%d\n", *puntual);
//	borrar_archivo_swap(1, fd);

}

int main(void) {

//	testeando_boludeces();

	inicializar_memoria();

	pthread_t hilo_kernel;
	pthread_create(&hilo_kernel, NULL, (void*) escuchar_kernel, NULL);
	pthread_detach(hilo_kernel);


	pthread_t hilo_cpu;
	pthread_create(&hilo_cpu, NULL, (void*) escuchar_cpu, NULL);
	pthread_join(hilo_cpu, NULL);

	return 0;

}
