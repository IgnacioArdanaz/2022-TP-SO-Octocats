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

/////////////////////12/5/2022///////////////////
//#include "kernel_utils.h"
//
//int semaforo_largo_plazo = 0;
//
//int s_exit_to_console = 0;
//
//int s_pid_exit = 1;
//
//char* pid_exit;
//
//int s_sockets_activos = 1;
//
//t_queue* cola_new;
//
//t_list* cola_ready;
//
//int semaforo_contador = 1;
//
//int contador_procesos = 1;
//
//t_dictionary* sockets_activos;
//
//// no se de donde sacar este dato asi que lo dejo aca temporalmente
//int grado_de_multip = 2;
//
//void exit_to_console(){
//
//	while(1){
//
//		//borrar referencias a memoria del proceso
//
//		wait(&s_exit_to_console);
//
//		wait(&s_sockets_activos);
//
//		int cliente_socket = dictionary_get(sockets_activos,pid_exit);
//		dictionary_remove(sockets_activos,pid_exit);
//
//		//send to console
//
//		signal(&s_sockets_activos);
//
//		wait(&semaforo_contador);
//		contador_procesos--;
//		signal(&semaforo_contador);
//
//		signal(s_pid_exit);
//
//	}
//}
//
//void planif_largo_plazo(){
//	while(1){
//		wait(&semaforo_largo_plazo);
//		if (contador_procesos < grado_de_multip){
//			PCB_t* next_pcb = queue_pop(cola_new);
//			list_add(cola_ready,next_pcb);
//
//			//sumamos 1 al grado de multiprogramacion actual, con
//			//sus semaforos correspondientes
//			wait(&semaforo_contador);
//			contador_procesos++;
//			signal(&semaforo_contador);
//		}
//	}
//	return;
//}
//
//void agregar_a_new(PCB_t pcb,int cliente_socket){
//	queue_push(cola_new,&pcb);
//	char* var_aux = "";
//	sprintf(var_aux,"%d",pcb.pid);
//	dictionary_put(sockets_activos,var_aux,&cliente_socket);
//}
//
//void inicializar_estructuras(){
//	cola_new = queue_create();
//	cola_ready = list_create();
//	pthread_t* hilo_planif_largo_plazo;
//	pthread_t* hilo_exit_to_console;
//	sockets_activos = dictionary_create();
//	pthread_create(hilo_planif_largo_plazo,NULL,(void*)planif_largo_plazo,NULL);
//	pthread_detach(hilo_planif_largo_plazo);
//	pthread_create(hilo_exit_to_console,NULL,(void*)exit_to_console,NULL);
//	pthread_detach(hilo_exit_to_console);
//}
//
//void procesar_socket(thread_args* argumentos){
//	t_log* logger = argumentos->log;
//	int cliente_socket = argumentos->cliente;
//	char* server_name = argumentos->server_name;
//	op_code cop;
//	while (cliente_socket != -1) {
//
//		if (recv(cliente_socket, &cop, sizeof(op_code), 0) <= 0) {
//			log_info(logger, "DISCONNECT_FAILURE!");
//			return;
//		}
//
//		switch (cop) {
//			case PROGRAMA:
//			{
//				char** instrucciones = string_array_new();
//				uint16_t tamanio;
//
//				if (!recv_programa(cliente_socket, &instrucciones, &tamanio)) {
//					log_error(logger, "Fallo recibiendo PROGRAMA");
//					break;
//				}
//
//				log_info(logger, "Tamanio %d", tamanio);
//
//
//				for(int i = 0; instrucciones[i] != NULL; i++){
//					log_info(logger, "Instruccion numero %d: %s \n", i, instrucciones[i]);
//				}
//
//
//				PCB_t proceso;
//				proceso.instrucciones = string_array_new();
//
//				proceso.pid = 1; // A DETERMINAR CON EL TEMA DE HILOS Y N CONSOLAS
//				proceso.tamanio = tamanio;
//				proceso.pc = 0;
//				proceso.instrucciones = instrucciones;
//				proceso.tabla_paginas = 0;   // SOLO LO INICIALIZAMOS, MEMORIA NOS VA A ENVIAR EL VALOR
//				proceso.est_rafaga = 5;  // ESTO VA POR ARCHIVO DE CONFIGURACION
//
//				agregar_a_new(proceso,cliente_socket);
//				signal(&semaforo_largo_plazo);
//
////				ip_cpu = config_get_string_value(config,"IP_CPU");
////				puerto_cpu_dispatch = config_get_string_value(config,"PUERTO_CPU_DISPATCH");
////
////				conexion_cpu_dispatch = crear_conexion(logger, "CPU DISPATCH", ip_cpu, puerto_cpu_dispatch);
////
////				send_proceso(conexion_cpu_dispatch, proceso);
//
//				string_array_destroy(instrucciones);
//
//				break;
//			}
//			case 2: //proceso terminado desde CPU
//			{
//				char** instrucciones = string_array_new();
//				uint16_t tamanio;
//
//				if (!recv_programa(cliente_socket, &instrucciones, &tamanio)) {
//					log_error(logger, "Fallo recibiendo FINISHED");
//					break;
//				}
//
//				PCB_t proceso;
//				wait(&s_pid_exit);
//				sprintf(pid_exit,"%d",proceso.pid);
//				signal(&s_exit_to_console);
//				break;
//			}
//
//			// Errores
//			case -1:
//				log_error(logger, "Cliente desconectado de kernel");
//				break;
//			default:
//				log_error(logger, "Algo anduvo mal en el server del kernel ");
//				log_info(logger, "Cop: %d", cop);
//		}
//	}
//
//	liberar_conexion(&cliente_socket);
//
//}
//
//int escuchar_servidor(t_log* logger, char* name, int server_socket){
//
//	int cliente_socket = esperar_cliente(logger, name, server_socket);
//	if (cliente_socket != -1){
//		pthread_t hilo;
//		thread_args* arg = malloc(sizeof(thread_args));
//		arg->log = logger;
//		arg->cliente = cliente_socket;
//		arg->server_name = name;
//		printf("Cliente socket: %d\n",cliente_socket);
//		pthread_create(&hilo,NULL,(void*)procesar_socket,(void*)arg);
//		pthread_detach(hilo);
//		return 1;
//	}
//	return 0;
//}



