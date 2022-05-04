#include "CPU.h"

int main() {

	t_log* logger = log_create("CPU.log", "CPU", 1, LOG_LEVEL_INFO);
	t_config* config = config_create("CPU.config");
	char* puerto_dispatch;
	char* puerto_interrupt;

	puerto_dispatch = config_get_string_value(config,"PUERTO_ESCUCHA_DISPATCH");
	int server_CPU_DISPACHT = iniciar_servidor(logger, "CPU", IP, puerto_dispatch);

	//int server_CPU_INTERRUPT = iniciar_servidor(logger, "CPU", IP, PUERTO_INTERRUPT);

	int cliente_socket_DISPACHT = esperar_cliente(logger, "CPU_DISPACHT",server_CPU_DISPACHT);

	//int cliente_socket_INTERRUPT = esperar_cliente(logger, "CPU_DISPACHT",server_CPU_DISPACHT);


	liberar_conexion(&cliente_socket_DISPACHT);
	log_destroy(logger);
	config_destroy(config);
	//Hay que iniciar dos servidores? o tenemos que cambiar la funcion iniciar_servidor para que escuche a 2 puertos?


}
