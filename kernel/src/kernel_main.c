#include "kernel_main.h"

int main(void) {
	t_log* logger = log_create("kernel.log", "kernel", 1, LOG_LEVEL_INFO);

	int server_fd = iniciar_servidor(logger, "KERNEL", IP, PUERTO);
	int cliente_fd = esperar_cliente(logger, "KERNEL",server_fd);

	return EXIT_SUCCESS;
}
