#include "cpu_main.h"

int main(void) {
	t_log* logger = log_create("cpu.log", "cpu", 1, LOG_LEVEL_INFO);
	t_config* config = config_create("cpu.config");
	char* puerto_dispatch;
	char* puerto_interrupt;

	puerto_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	int server_cpu_dispatch = iniciar_servidor(logger, "CPU_DISPATCH", IP, puerto_dispatch);

	//puerto_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
	//int server_cpu_interrupt = iniciar_servidor(logger, "CPU_INTERRUPT", IP, puerto_interrupt);

	int cliente_socket_dispatch = esperar_cliente(logger, "CPU_DISPACHT",server_cpu_dispatch);

	//int cliente_socket_interrupt = esperar_cliente(logger, "CPU_INTERRUPT", server_cpu_interrupt);

	op_code cop;
	while (cliente_socket_dispatch != -1) {

//		if (recv(cliente_socket_dispatch, &cop, sizeof(op_code), 0) <= 0) {
//			log_info(logger, "DISCONNECT!");
//			return EXIT_FAILURE;
//		}
		cop = PROCESO;
		switch (cop) {
			case PROCESO:
			{
				PCB_t proceso;
				proceso.instrucciones = string_array_new();

				if (!recv_proceso(cliente_socket_dispatch, &proceso)) {
					log_error(logger, "Fallo recibiendo PROCESO");
					break;
				}

				log_info(logger, "Ejecutando proceso pid=%d", proceso.pid);
				for(int i = 0; (proceso.instrucciones)[i] != NULL; i++){
					log_info(logger, "Instruccion numero %d: %s \n", i, (proceso.instrucciones)[i]);
				}
				log_info(logger, "PCB ---> pid=%d | tamanio=%d | pc=%d | tabla_paginas=%d | est_rafaga=%d", proceso.pid, proceso.tamanio, proceso.pc, proceso.tabla_paginas, proceso.est_rafaga );
				break;
			}
			default:
				log_error(logger, "Algo anduvo mal en el server del kernel ");
				log_info(logger, "Cop: %d", cop);
				return EXIT_FAILURE;
		}
	}

	liberar_conexion(&cliente_socket_dispatch);
	//liberar_conexion(&cliente_socket_interrupt);
	log_destroy(logger);
	config_destroy(config);

	return EXIT_SUCCESS;
}
