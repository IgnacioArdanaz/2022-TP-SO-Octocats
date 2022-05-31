#include "memoria_main.h"

char* puerto_escucha;
int cliente_socket;

int main(void) {

	inicializar_memoria();

	puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA_DISPATCH");
	int server_memoria = iniciar_servidor(logger, "MEMORIA", IP, puerto_escucha);

	cliente_socket = esperar_cliente(logger, "MEMORIA",server_memoria);

	apagar_memoria();

	return EXIT_SUCCESS;

}

void inicializar_memoria(){
	logger = log_create("memoria.log", "memoria", 1, LOG_LEVEL_INFO);
	config = config_create("memoria.config");
}

void apagar_memoria(){
	liberar_conexion(&cliente_socket);
	log_destroy(logger);
	config_destroy(config);
}
