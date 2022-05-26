#include "kernel_utils.h"

pthread_mutex_t mx_multip_actual = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_pid_sig = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_logger = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cpu_desocupado = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cola_new = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_lista_ready = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_socket = PTHREAD_MUTEX_INITIALIZER;

sem_t s_new_ready, s_fifo_ready_execute, s_srt_ready_execute, s_cont_ready;

t_dictionary* sockets;
t_queue* cola_new;
t_list* cola_ready;
t_queue* cola_blocked;

int pid_sig, estimacion_inicial, grado_multiprogramacion, multiprogramacion_actual, conexion_cpu_dispatch, cpu_desocupado = 1, aaa=1;
char *algoritmo_config, *ip_cpu, *puerto_cpu_dispatch;

algoritmo_t algoritmo;

void inicializar(){
	logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);
	config = config_create("kernel.config");

	cola_new = queue_create();
	cola_blocked = queue_create();
	cola_ready = list_create();

	sockets = dictionary_create();

	pid_sig = 1;

	grado_multiprogramacion = config_get_int_value(config,"GRADO_MULTIPROGRAMACION");
	estimacion_inicial = config_get_double_value(config,"ESTIMACION_INICIAL");
	ip_cpu = config_get_string_value(config,"IP_CPU");
	puerto_cpu_dispatch = config_get_string_value(config,"PUERTO_CPU_DISPATCH");
	algoritmo_config = config_get_string_value(config,"ALGORITMO_PLANIFICACION");

	conexion_cpu_dispatch = crear_conexion(logger, "CPU DISPATCH", ip_cpu, puerto_cpu_dispatch);

	sem_init(&s_new_ready, 0, 0);
	sem_init(&s_cont_ready, 0, 0); // Incrementa al sumar un proceso a ready y decrementa al ejecutarlo

	pthread_t hilo_pasaje_new_ready;
	pthread_create(&hilo_pasaje_new_ready,NULL,(void*)pasaje_new_ready,NULL);
	pthread_detach(hilo_pasaje_new_ready);

	if(strcmp(algoritmo_config, "FIFO") == 0){
		algoritmo = FIFO;
	} else {
		algoritmo = SRT;
	}
	switch(algoritmo){
		case FIFO:
		{
			printf("FIFO %d", algoritmo);
			sem_init(&s_fifo_ready_execute, 0, 0);
			pthread_t hilo_fifo_ready_execute;
			pthread_create(&hilo_fifo_ready_execute,NULL,(void*)fifo_ready_execute,NULL);
			pthread_detach(hilo_fifo_ready_execute);
			break;
		}
		case SRT:
		{
			printf("SRT %d", algoritmo);
			sem_init(&s_srt_ready_execute, 0, 0);
			pthread_t hilo_srt_ready_execute;
			pthread_create(&hilo_srt_ready_execute,NULL,(void*)srt_ready_execute,NULL);
			pthread_detach(hilo_srt_ready_execute);
			break;
		}
	}

}

int escuchar_servidor(char* name, int server_socket){
	printf("Kernel esperando un nuevo cliente \n");
	int cliente_socket = esperar_cliente(logger, name, server_socket);
	printf("Socket del cliente recibido en kernel.\n ");
	if (cliente_socket != -1){
		pthread_t hilo;
		thread_args* arg = malloc(sizeof(thread_args));
		arg->cliente = cliente_socket;
		arg->server_name = name;
		pthread_create(&hilo, NULL, (void*) procesar_socket, (void*) arg);
		pthread_detach(hilo);
		printf("Cliente procesado por kernel\n");
		return 1;
	}
	return 0;
}

void procesar_socket(thread_args* argumentos){
	int cliente_socket = argumentos->cliente;
	char* server_name = argumentos->server_name;
	uint32_t resultOk = 0;
	int32_t resultError = -1;
	free(argumentos);
	op_code cop;
	while (cliente_socket != -1) {
		if (recv(cliente_socket, &cop, sizeof(op_code), 0) <= 0) {
			pthread_mutex_lock(&mx_logger);
			log_info(logger, "DISCONNECT_FAILURE!");
			pthread_mutex_unlock(&mx_logger);
			send(cliente_socket, &resultError, sizeof(uint32_t), NULL);
			return;
		}
		switch (cop) {
			case PROGRAMA:
			{
				char** instrucciones = string_array_new();
				uint16_t tamanio = 0;

				if (!recv_programa(cliente_socket, &instrucciones, &tamanio)) {
					pthread_mutex_lock(&mx_logger);
					log_error(logger, "Fallo recibiendo PROGRAMA");
					pthread_mutex_unlock(&mx_logger);
					break;
				}

				pthread_mutex_lock(&mx_logger);
				log_info(logger, "Tamanio %d", tamanio);
				pthread_mutex_unlock(&mx_logger);

				for(int i = 0; i <  string_array_size(instrucciones); i++){
					pthread_mutex_lock(&mx_logger);
					log_info(logger, "Instruccion numero %d: %s", i, instrucciones[i]);
					pthread_mutex_unlock(&mx_logger);
				}

				PCB_t* proceso = malloc(sizeof(PCB_t));
				proceso->instrucciones = string_array_new();

				pthread_mutex_lock(&mx_pid_sig);
				proceso->pid = pid_sig;
				pid_sig+=1;
				pthread_mutex_unlock(&mx_pid_sig);

				proceso->tamanio = tamanio;
				proceso->pc = 0;
				proceso->instrucciones = instrucciones;
				proceso->tabla_paginas = 0;   // SOLO LO INICIALIZAMOS, MEMORIA NOS VA A ENVIAR EL VALOR
				proceso->est_rafaga = estimacion_inicial;  // ESTO VA POR ARCHIVO DE CONFIGURACION

				pthread_mutex_lock(&mx_cola_new);
				queue_push(cola_new, proceso);
				pthread_mutex_unlock(&mx_cola_new);

				sem_post(&s_new_ready); //Avisa al hilo planificador de pasaje de new a ready que debe ejecutarse.

				string_array_destroy(instrucciones);

//				send(cliente_socket, &resultOk, sizeof(uint32_t), NULL); // Forma de avisar a la consola, no iria aca

				return;
			}
			//si la variable que evaluamos en el switch es un op_code
			//que tiene de posibilidades definidas 0 (PROGRAMA) o 1 (PROCESO)
			//cuando en la vida puede llegar a dar -1???
			// Errores
//			case -1:
//				log_error(logger, "Cliente desconectado de kernel");
//				break;
			default:
				pthread_mutex_lock(&mx_logger);
				log_error(logger, "Algo anduvo mal en el server del kernel ");
				log_info(logger, "Cop: %d", cop);
				pthread_mutex_unlock(&mx_logger);
		}
	}

	liberar_conexion(&cliente_socket);

}

/****Hilo NEW -> READY ***/
void pasaje_new_ready(){

	grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
	while(1){
		sem_wait(&s_new_ready);
		pthread_mutex_lock(&mx_cola_new);
		//AGREGAR QUE SI HAY PCBS EN READY SUSPENDED TIENEN PRIORIDAD DE PASO
		if(multiprogramacion_actual < grado_multiprogramacion){
			PCB_t* p = queue_pop(cola_new);
			list_add(cola_ready,p);
			sem_post(&s_cont_ready); //Sumo uno al contador de ready
			pthread_mutex_lock(&mx_multip_actual);
			multiprogramacion_actual++;
			pthread_mutex_unlock(&mx_multip_actual);
			pthread_mutex_lock(&mx_logger);
			log_info(logger,"(new -> ready) Cola de new: %d",queue_size(cola_new));
			log_info(logger,"(new -> ready) Cola de ready: %d", list_size(cola_ready));
			pthread_mutex_unlock(&mx_logger);
			switch(algoritmo){
				case FIFO:
				{
					sem_post(&s_fifo_ready_execute);
					break;
				}
				case SRT:
				{
					sem_post(&s_srt_ready_execute);
					break;
				}
			}
		}
		pthread_mutex_unlock(&mx_cola_new);
	}

}

/****Hilo READY -> EXECUTE (FIFO) ***/
void fifo_ready_execute(){
	while(1){
		sem_wait(&s_fifo_ready_execute);
		pthread_mutex_lock(&mx_cpu_desocupado);
		if(cpu_desocupado){ // Para que no ejecute cada vez que un proceso pasa de new a ready
			sem_wait(&s_cont_ready); // Para que no intente ejecutar si la lista de ready esta vacia
			PCB_t* proceso = malloc(sizeof(PCB_t));
			pthread_mutex_lock(&mx_lista_ready);
			proceso = list_remove(cola_ready, 0);
			pthread_mutex_unlock(&mx_lista_ready);
			pthread_mutex_lock(&mx_logger);
			log_info(logger, "\n Mandando proceso %d a ejecutar tam %d inst %s %s %s %s pc %d tabla %d est %d \n", proceso->pid, proceso->tamanio, proceso->instrucciones[0], proceso->instrucciones[1], proceso->instrucciones[2], proceso->instrucciones[3], proceso->pc, proceso->tabla_paginas, proceso->est_rafaga);
			pthread_mutex_unlock(&mx_logger);
			cpu_desocupado = 0;
			//send_proceso(conexion_cpu_dispatch, proceso);
			free(proceso);
			}
		pthread_mutex_unlock(&mx_cpu_desocupado);
	}
}

/****Hilo READY -> EXECUTE (SRT) ***/
void srt_ready_execute(){
	while(1){
		sem_wait(&s_srt_ready_execute);
//		pthread_mutex_lock(&mx_cola_new);
//		//AGREGAR QUE SI HAY PCBS EN READY SUSPENDED TIENEN PRIORIDAD DE PASO
//		if(multiprogramacion_actual < grado_multiprogramacion){
//			PCB_t* p = queue_pop(cola_new);
//			list_add(cola_ready,p);
//			pthread_mutex_lock(&mx_multip_actual);
//			multiprogramacion_actual++;
//			pthread_mutex_unlock(&mx_multip_actual);
//		}
//		pthread_mutex_unlock(&mx_cola_new);
	}

}

//Este no va
PCB_t* fifo(){
	return list_remove(cola_ready,0);
}

//Este no va pero se puede usar la logica para hacer srt_ready_execute
PCB_t* sjf(){

	PCB_t* primer_pcb = list_get(cola_ready,0);
	double raf_min = primer_pcb->est_rafaga;
	int index_pcb = 0;
	int i;
	for (i = 1;i<list_size(cola_ready);i++){
		PCB_t* pcb = list_get(cola_ready,i);
		double est_raf = pcb->est_rafaga;
		//si el sjf luego sigue con FIFO, entonces tiene que ser >
		//si el sjf luego sigue con LIFO, entonces tiene que ser >=
		if(raf_min >= est_raf){
			raf_min = est_raf;
			index_pcb = i;
		}
	}
	return list_remove(cola_ready,index_pcb);

}
