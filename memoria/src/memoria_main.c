#include "memoria_main.h"

char* puerto_escucha;
int cliente_cpu, cliente_kernel;

int main(void) {

	inicializar_memoria();

	puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA");
	int server_memoria = iniciar_servidor(logger, "MEMORIA", IP, puerto_escucha);

	cliente_cpu = esperar_cliente(logger, "MEMORIA", server_memoria);
	cliente_kernel = esperar_cliente(logger, "MEMORIA", server_memoria);

	while(true); //simula que sigue corriendo

	apagar_memoria();

	return EXIT_SUCCESS;

}
