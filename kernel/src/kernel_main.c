#include "kernel_main.h"

int main(void) {
	t_log* logger = log_create("kernel.log", "kernel", 1, LOG_LEVEL_INFO);

	int server_socket = iniciar_servidor(logger, "KERNEL", IP, PUERTO);
	int cliente_socket = esperar_cliente(logger, "KERNEL",server_socket);

	op_code cop;
	while (cliente_socket != -1) {

		if (recv(cliente_socket, &cop, sizeof(op_code), 0) <= 0) {
			log_info(logger, "DISCONNECT!");
			return EXIT_FAILURE;
		}

		switch (cop) {
			case PROGRAMA:
			{
				char** instrucciones = string_array_new();
				uint8_t tamanio;



				if (!recv_programa(cliente_socket, &instrucciones, &tamanio)) {
					log_error(logger, "Fallo recibiendo PROGRAMA");
					break;
				}

				log_info(logger, "Tamanio %d", tamanio);


				for(int i = 0; instrucciones[i] != NULL; i++){
					log_info(logger, "Instruccion numero %d: %s \n", i, instrucciones[i]);
				}

				string_array_destroy(instrucciones);
				break;
			}

			// Errores
			case -1:
				log_error(logger, "Cliente desconectado de kernel");
				return EXIT_FAILURE;
			default:
				log_error(logger, "Algo anduvo mal en el server del kernel ");
				log_info(logger, "Cop: %d", cop);
				return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
