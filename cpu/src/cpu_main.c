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

	fd_set readSet;
	fd_set tempReadSet;
	int maximoFD;
	int resultado;

	int fd;
	int nuevaConexion;

	FD_ZERO(&readSet);
	FD_ZERO(&tempReadSet);

	FD_SET(server_cpu_dispatch,&readSet);
	maximoFD = server_cpu_dispatch;
	int i = 0;
	while(i < 15) //1)  //OJO ACA !!!
	{
		FD_ZERO(&tempReadSet);
		tempReadSet = readSet; //NOSE SI ESTA BIEN ASI DE PREPO
		//memset(tempReadSet,readSet,sizeof(readSet));

		//memcpy(&tempReadSet, &readSet, sizeof(tempReadSet));
		resultado = select(maximoFD + 1, &tempReadSet, NULL, NULL, NULL);
		//printf("\nResultadoSELECT: %d\n",resultado); ///

		if(resultado == -1)
		{
			printf("Fallo en select().\n");
		}
		//VERIFICO SI HAY NUEVA CONEXION
		if (FD_ISSET(server_cpu_dispatch, &tempReadSet))
		{
			nuevaConexion = esperar_cliente(logger, "CPU_DISPACHT",server_cpu_dispatch);
			FD_SET(nuevaConexion,&readSet);
			maximoFD = (nuevaConexion > maximoFD) ? nuevaConexion : maximoFD;
			FD_CLR(server_cpu_dispatch,&tempReadSet); //LO SACO PARA Q SOLO ME QUEDEN FDs DE CONEXIONES PARA RECIVIR ALGUN PAQUETE
		}
		//BUSCO EN EL CONJUNTO, si hay datos para leer
		for(fd = 0; fd <= maximoFD; fd++)
		{
			if (FD_ISSET(fd, &tempReadSet)) //HAY UN PAQUETE PARA RECIBIR
			{
				op_code cop;

				if (recv(fd, &cop, sizeof(op_code), 0) <= 0) {
					log_info(logger, "DISCONNECT!");
					return EXIT_FAILURE;
				}

				switch (cop) {
					case PROCESO:
					{
						PCB_t* proceso = malloc(sizeof(PCB_t));
						proceso->instrucciones = list_create();

						if (!recv_proceso(fd, proceso)) {
							log_error(logger, "Fallo recibiendo PROCESO");
							break;
						}

						log_info(logger, "Ejecutando proceso pid=%d", proceso->pid);
						for(int i = 0; i < list_size(proceso->instrucciones); i++){
							instruccion_t* inst = list_get(proceso->instrucciones,i);
							printf("%c %d %d\n",inst->operacion,inst->arg1,inst->arg2);
						}
						log_info(logger, "PCB ---> pid=%d | tamanio=%d | pc=%d | tabla_paginas=%d | est_rafaga=%d", proceso->pid, proceso->tamanio, proceso->pc, proceso->tabla_paginas, proceso->est_rafaga );
						break;
					}
					default:
						log_error(logger, "Algo anduvo mal en el server del kernel ");
						log_info(logger, "Cop: %d", cop);
						return EXIT_FAILURE;
				}

			}
			FD_CLR(fd,&tempReadSet);
		}
		i++;
	}

	/* VIEJO QUE NO FUNCIONA MULTIPLES CONEXIONES ****************************************************+

	int cliente_socket_dispatch = esperar_cliente(logger, "CPU_DISPACHT",server_cpu_dispatch);

	//int cliente_socket_interrupt = esperar_cliente(logger, "CPU_INTERRUPT", server_cpu_interrupt);

	op_code cop;
	while (cliente_socket_dispatch != -1) {

		if (recv(cliente_socket_dispatch, &cop, sizeof(op_code), 0) <= 0) {
			log_info(logger, "DISCONNECT!");
			return EXIT_FAILURE;
		}
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


	*******************************************************************************************************************/

//	liberar_conexion(&cliente_socket_dispatch);
	//liberar_conexion(&cliente_socket_interrupt);
	log_destroy(logger);
	config_destroy(config);

	return EXIT_SUCCESS;
}
