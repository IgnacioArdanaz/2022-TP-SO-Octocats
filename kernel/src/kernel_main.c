#include "kernel_main.h"


int main(void) {

	inicializar_kernel();
	t_config* config_ips = config_create("../../ips.config");
	char* ip = config_get_string_value(config_ips,"IP_KERNEL");
	printf("Valor %s\n", ip);
	char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	int server_socket = iniciar_servidor(logger, "KERNEL", ip, puerto_escucha);
	int result = EXIT_FAILURE;
	config_destroy(config_ips);
	if (server_socket == 0){
		log_error(logger,"Error creando el socket :(");
		printf("Error creando el socket :(");
		result = EXIT_FAILURE;
	} else while(escuchar_servidor("KERNEL",server_socket));

	return result;
}
