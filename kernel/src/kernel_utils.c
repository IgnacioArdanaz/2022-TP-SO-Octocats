#include "kernel_utils.h"

void inicializar(){
	logger = log_create("kernel.log", "kernel", 1, LOG_LEVEL_INFO);
	config = config_create("kernel.config");
}

void procesar_socket(thread_args* argumentos){
	int cliente_socket = argumentos->cliente;
	char* server_name = argumentos->server_name;
	op_code cop;
	while (cliente_socket != -1) {

		if (recv(cliente_socket, &cop, sizeof(op_code), 0) <= 0) {
			log_info(logger, "DISCONNECT_FAILURE!");
			return;
		}

		switch (cop) {
			case PROGRAMA:
			{
				char** instrucciones = string_array_new();
				uint16_t tamanio;

				if (!recv_programa(cliente_socket, &instrucciones, &tamanio)) {
					log_error(logger, "Fallo recibiendo PROGRAMA");
					break;
				}

				log_info(logger, "Tamanio %d", tamanio);


				for(int i = 0; instrucciones[i] != NULL; i++){
					log_info(logger, "Instruccion numero %d: %s \n", i, instrucciones[i]);
				}


				PCB_t proceso;
				proceso.instrucciones = string_array_new();

				proceso.pid = 1; // A DETERMINAR CON EL TEMA DE HILOS Y N CONSOLAS
				proceso.tamanio = tamanio;
				proceso.pc = 0;
				proceso.instrucciones = instrucciones;
				proceso.tabla_paginas = 0;   // SOLO LO INICIALIZAMOS, MEMORIA NOS VA A ENVIAR EL VALOR
				proceso.est_rafaga = 5;  // ESTO VA POR ARCHIVO DE CONFIGURACION

//				ip_cpu = config_get_string_value(config,"IP_CPU");
//				puerto_cpu_dispatch = config_get_string_value(config,"PUERTO_CPU_DISPATCH");
//
//				conexion_cpu_dispatch = crear_conexion(logger, "CPU DISPATCH", ip_cpu, puerto_cpu_dispatch);
//
//				send_proceso(conexion_cpu_dispatch, proceso);

				string_array_destroy(instrucciones);

				break;
			}

			// Errores
			case -1:
				log_error(logger, "Cliente desconectado de kernel");
			default:
				log_error(logger, "Algo anduvo mal en el server del kernel ");
				log_info(logger, "Cop: %d", cop);
		}
	}

	liberar_conexion(&cliente_socket);

}

int escuchar_servidor(char* name, int server_socket){

	int cliente_socket = esperar_cliente(logger, name, server_socket);
	if (cliente_socket != -1){
		pthread_t hilo;
		thread_args* arg = malloc(sizeof(thread_args));
		arg->cliente = cliente_socket;
		arg->server_name = name;
		printf("Cliente socket: %d\n",cliente_socket);
		pthread_create(&hilo,NULL,(void*)procesar_socket,(void*)arg);
		pthread_detach(hilo);
		return 1;
	}
	return 0;
}
