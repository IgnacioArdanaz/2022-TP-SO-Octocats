#include "kernel_utils.h"

int multiprogramacion_actual = 0;

void procesar_consola(procesar_consola_args* argumentos){
	t_log* logger = argumentos->log;
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

/****************ESTO LO VA A HACER LA COLA DE READY********************************************************/

//				ip_cpu = config_get_string_value(config,"IP_CPU");
//				puerto_cpu_dispatch = config_get_string_value(config,"PUERTO_CPU_DISPATCH");
//
//				conexion_cpu_dispatch = crear_conexion(logger, "CPU DISPATCH", ip_cpu, puerto_cpu_dispatch);
//
//				send_proceso(conexion_cpu_dispatch, proceso);
/**********************************************************************************************************/

/****************A PARTIR DE ACA, VIENE EL POSIBLE CODIGO DE GESTION DE LA COLA DE NEW*********************/

//				queue_push(argumentos->colaNew, (void*) proceso);

//				pthread_t hilo_pasaje_new_ready;
//				procesar_consola_args* arg = malloc(sizeof(procesar_consola_args));
//				arg->log = logger;
//				arg->colaNew = argumentos->colaNew;
//				pthread_create(&hilo_pasaje_new_ready,NULL,(void*)pasaje_new_ready,(void*)arg);
//				pthread_detach(hilo_pasaje_new_ready);
/**********************************************************************************************************/

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

int escuchar_servidor(t_log* logger, char* name, int server_socket, t_queue* colaNew){

	int cliente_socket = esperar_cliente(logger, name, server_socket);
	if (cliente_socket != -1){
		pthread_t hilo_procesar_consola;
		procesar_consola_args* arg = malloc(sizeof(procesar_consola_args));
		arg->log = logger;
		arg->cliente = cliente_socket;
		arg->server_name = name;
		arg->colaNew = colaNew;
		printf("Cliente socket: %d\n",cliente_socket);
		pthread_create(&hilo_procesar_consola,NULL,(void*)procesar_consola,(void*)arg);
		pthread_detach(hilo_procesar_consola);
		return 1;
	}
	return 0;
}

int pasaje_new_ready(pasaje_new_ready_args* argumentos){
	t_log* logger = argumentos->log;
	t_config* config = config_create("kernel.config");
	int grado_multiprogramacion = config_get_string_value(config, "GRADO_MULTIPROGRAMACION");

	if(multiprogramacion_actual < grado_multiprogramacion){

	}


	return 0;
}



