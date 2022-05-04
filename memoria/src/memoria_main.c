#include "memoria_main.h"

int main(void) {
	t_log* logger = log_create("memoria.log", "memoria", 1, LOG_LEVEL_INFO);
	t_config* config = config_create("memoria.config");
	char* puerto_escucha;

	puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA_DISPATCH");
	int server_memoria = iniciar_servidor(logger, "MEMORIA", IP, puerto_escucha);


	int cliente_socket_memoria = esperar_cliente(logger, "MEMORIA",server_memoria);



	liberar_conexion(&cliente_socket_memoria);
	log_destroy(logger);
	config_destroy(config);
}
