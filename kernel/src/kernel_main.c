#include "kernel_main.h"


int main(void) {

	int conexion_cpu_dispatch;
	int conexion_memoria;
	char* puerto_escucha;
	char* ip_cpu;
	char* puerto_cpu_dispatch;
	char* puerto_cpu_interrupt;
	char* puerto_memoria;

	t_log* logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);
	t_config* config = config_create("kernel.config");

	inicializar(logger, config);
	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	int server_socket = iniciar_servidor(logger, "KERNEL", IP, puerto_escucha);
	int result = 5;
	if (server_socket == 0){
//		log_error(logger,"Error creando el socket :(");
		printf("Error creando el socket :(");
		result = EXIT_FAILURE;
	} else{
		while(escuchar_servidor("KERNEL",server_socket, logger));
		printf("DISCONNECT_SUCCESS!");
		//log_info(logger, "DISCONNECT_SUCCESS!");
	}

	log_destroy(logger);
	config_destroy(config);

	return result;
}
