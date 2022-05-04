#include "kernel_main.h"

int main(void) {
	t_log* logger;
	t_config* config;
	int conexion_CPU;
	int conexion_memoria;
	char* puerto_escucha;
	char* ip_cpu;
	char* puerto_cpu_dispatch;
	char* puerto_cpu_interrupt;
	char* puerto_memoria;


	logger = log_create("kernel.log", "kernel", 1, LOG_LEVEL_INFO);
	config = config_create("kernel.config");


	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");

	int server_socket = iniciar_servidor(logger, "KERNEL", IP, puerto_escucha);
	int cliente_socket = esperar_cliente(logger, "KERNEL",server_socket);

	op_code cop;
	while (cliente_socket != -1) {

		if (recv(cliente_socket, &cop, sizeof(op_code), 0) <= 0) {
			log_info(logger, "DISCONNECT_FAILURE!");
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
				liberar_conexion(&cliente_socket);

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

	log_info(logger, "DISCONNECT_SUCCESS!");



	ip_cpu= config_get_string_value(config,"IP_CPU");
	puerto_cpu_dispatch = config_get_string_value(config,"PUERTO_CPU_DISPATCH");
	conexion_CPU = crear_conexion(logger, "CPU DISPATCH", ip_cpu, puerto_cpu_dispatch);



	log_destroy(logger);
	config_destroy(config);

	return EXIT_SUCCESS;
}
