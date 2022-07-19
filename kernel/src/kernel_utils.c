#include "kernel_utils.h"

pthread_mutex_t mx_pid_sig = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cola_new = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_lista_ready = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_socket = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_log = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cola_blocked = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cola_suspended_blocked = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cola_suspended_ready = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_iteracion_blocked = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cpu_desocupado = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_hay_interrupcion = PTHREAD_MUTEX_INITIALIZER;


sem_t s_pasaje_a_ready, s_ready_execute, s_cont_ready, s_cpu_desocupado, s_blocked,
	s_suspended_ready, s_multiprogramacion_actual, s_pcb_desalojado, s_esperar_cpu;

t_dictionary* sockets;
t_queue* cola_new;
t_list* lista_ready;
t_list* cola_blocked;
t_list* cola_suspended_blocked;
t_queue* cola_suspended_ready;
t_dictionary* iteracion_blocked;

bool cpu_desocupado;
bool hay_interrupcion;

int pid_sig, conexion_cpu_dispatch, conexion_cpu_interrupt, tiempo_suspended, conexion_memoria;

double estimacion_inicial;

void inicializar_kernel(){

	logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);
	config = config_create("../kernel.config");

	cola_new = queue_create();
	cola_blocked = list_create();
	lista_ready = list_create();
	cola_suspended_blocked = list_create();
	cola_suspended_ready = queue_create();
	sockets = dictionary_create();
	iteracion_blocked = dictionary_create();

	pid_sig = 1;
	cpu_desocupado = true;
	hay_interrupcion = false;

	estimacion_inicial = config_get_double_value(config,"ESTIMACION_INICIAL");
	char* algoritmo_config = config_get_string_value(config,"ALGORITMO_PLANIFICACION");
	tiempo_suspended = config_get_int_value(config,"TIEMPO_MAXIMO_BLOQUEADO");

	t_config* config_ips = config_create("../../ips.config");
	char* ip_memoria = config_get_string_value(config_ips,"IP_MEMORIA");
	char* ip_cpu = config_get_string_value(config_ips,"IP_CPU");

	//conectando con MEMORIA
	//char* ip_memoria = config_get_string_value(config,"IP_MEMORIA");
	char* puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");
	conexion_memoria = crear_conexion(logger, "MEMORIA", ip_memoria, puerto_memoria);

	if (conexion_memoria == 0){
		log_error(logger,"Error al intentar conectarse a memoria :-(");
		exit(EXIT_FAILURE);
	}


	//conectando con CPU
	//char* ip_cpu = config_get_string_value(config,"IP_CPU");
	char* puerto_cpu_dispatch = config_get_string_value(config,"PUERTO_CPU_DISPATCH");
	char* puerto_cpu_interrupt = config_get_string_value(config,"PUERTO_CPU_INTERRUPT");
	conexion_cpu_dispatch = crear_conexion(logger, "CPU DISPATCH", ip_cpu, puerto_cpu_dispatch);
	conexion_cpu_interrupt= crear_conexion(logger, "CPU INTERRUPT", ip_cpu, puerto_cpu_interrupt);
	double alfa = config_get_double_value(config,"ALFA");
	send(conexion_cpu_dispatch,&alfa,sizeof(alfa),0);

	config_destroy(config_ips);
	sem_init(&s_pasaje_a_ready, 0, 0);
	sem_init(&s_cont_ready, 0, 0); // Incrementa al sumar un proceso a ready y decrementa al ejecutarlo
	sem_init(&s_blocked, 0, 0);
	sem_init(&s_suspended_ready, 0, 0);
	sem_init(&s_cpu_desocupado, 0, 1);
	sem_init(&s_multiprogramacion_actual, 0, config_get_int_value(config,"GRADO_MULTIPROGRAMACION"));
	sem_init(&s_ready_execute, 0, 0);
	sem_init(&s_pcb_desalojado, 0, 0);
	sem_init(&s_esperar_cpu, 0, 0);

	pthread_t hilo_pasaje_a_ready;
	pthread_create(&hilo_pasaje_a_ready,NULL,(void*)pasaje_a_ready,NULL);
	pthread_detach(hilo_pasaje_a_ready);

	pthread_t hilo_blocked;
	pthread_create(&hilo_blocked,NULL,(void*)ejecutar_io,NULL);
	pthread_detach(hilo_blocked);

	pthread_t hilo_esperar_cpu;
	pthread_create(&hilo_esperar_cpu,NULL,(void*)esperar_cpu,NULL);
	pthread_detach(hilo_esperar_cpu);

	pthread_t hilo_ready_execute;
	if (strcmp(algoritmo_config, "FIFO") == 0)
		pthread_create(&hilo_ready_execute,NULL,(void*)fifo_ready_execute,NULL);
	else if (strcmp(algoritmo_config, "SRT") == 0)
		pthread_create(&hilo_ready_execute,NULL,(void*)srt_ready_execute,NULL);
	else{ //si no es ni FIFO ni SRT, loggea el error y sale del programa
		log_error(logger,"Error en la configuracion: \"ALGORITMO_PLANIFICACION\" debe ser \"FIFO\" o \"SRT\"");
		exit(-1);
	}
	pthread_detach(hilo_ready_execute);

}

int escuchar_servidor(char* name, int server_socket){
	log_info(logger,"Kernel esperando un nuevo cliente");
	int cliente_socket = esperar_cliente(logger, name, server_socket);
	log_info(logger,"Socket del cliente recibido en kernel.");
	if (cliente_socket != -1){
		pthread_t hilo;
		thread_args* arg = malloc(sizeof(thread_args));
		arg->cliente = cliente_socket;
		arg->server_name = name;
		pthread_create(&hilo, NULL, (void*) procesar_socket, (void*) arg);
		pthread_detach(hilo);
		return 1;
	}
	return 0;
}

void procesar_socket(thread_args* argumentos){
	int cliente_socket = argumentos->cliente;
	//char* server_name = string_duplicate(argumentos->server_name);
	free(argumentos);
	op_code cop;
	while (cliente_socket != -1) {
		if (recv(cliente_socket, &cop, sizeof(op_code), 0) <= 0) {
			pthread_mutex_lock(&mx_log);
			log_error(logger,"DISCONNECT FAILURE!");
			pthread_mutex_unlock(&mx_log);
			int32_t resultError = -1;
			send(cliente_socket, &resultError, sizeof(int32_t), 0);
			exit(-1);
		}
		switch (cop) {
			case PROGRAMA:
			{
				t_list* instrucciones = list_create();
				uint16_t tamanio = 0;

				if (!recv_programa(cliente_socket, instrucciones, &tamanio)) {
					pthread_mutex_lock(&mx_log);
					log_error(logger,"Fallo recibiendo PROGRAMA");
					pthread_mutex_unlock(&mx_log);
					break;
				}

				pthread_mutex_lock(&mx_pid_sig);
				uint16_t pid_actual = pid_sig;
				pid_sig+=1;
				pthread_mutex_unlock(&mx_pid_sig);
				PCB_t* proceso = pcb_create();
				pcb_set(proceso, pid_actual, tamanio, instrucciones, 0, 0, estimacion_inicial);

				pthread_mutex_lock(&mx_cola_new);
				queue_push(cola_new, proceso);
				pthread_mutex_unlock(&mx_cola_new);

				char* key = string_itoa(pid_actual);
				pthread_mutex_lock(&mx_socket);
				dictionary_put(sockets, key, (int*) cliente_socket);
				pthread_mutex_unlock(&mx_socket);
				pthread_mutex_lock(&mx_iteracion_blocked);
				dictionary_put(iteracion_blocked, key, 0);
				pthread_mutex_unlock(&mx_iteracion_blocked);
				sem_post(&s_pasaje_a_ready); //Avisa al hilo planificador de pasaje a ready que debe ejecutarse.
				free(key);
				return;
			}

			default:
				pthread_mutex_lock(&mx_log);
				log_error(logger, "Algo anduvo mal en el server del kernel\n Cop: %d",cop);
				pthread_mutex_unlock(&mx_log);
		}
	}

	liberar_conexion(&cliente_socket);

}

/****Hilo NEW -> READY ||| SUSPENDED_READY -> READY***/
void pasaje_a_ready(){
	while(1){
		sem_wait(&s_pasaje_a_ready);
		sem_wait(&s_multiprogramacion_actual);
		PCB_t* proceso;
		pthread_mutex_lock(&mx_cola_suspended_ready);
		bool hay_procesos_en_suspended_ready = queue_is_empty(cola_suspended_ready);
		pthread_mutex_unlock(&mx_cola_suspended_ready);
		if(hay_procesos_en_suspended_ready){ //Si no hay suspendidos, agarro uno de new
			pthread_mutex_lock(&mx_cola_new);
			proceso = queue_pop(cola_new);
			pthread_mutex_unlock(&mx_cola_new);
			log_info(logger,"[NEW -> READY] PROCESO %d AGREGADO A READY", proceso->pid);
			solicitar_tabla_paginas(proceso);
		}
		else{
			pthread_mutex_lock(&mx_cola_suspended_ready);
			proceso = queue_pop(cola_suspended_ready);
			log_info(logger,"[SUSP_READY -> READY] PROCESO %d AGREGADO A READY", proceso->pid);
			pthread_mutex_unlock(&mx_cola_suspended_ready);
		}
		pthread_mutex_lock(&mx_lista_ready);
		list_add(lista_ready, proceso);
		pthread_mutex_unlock(&mx_lista_ready);
		sem_post(&s_ready_execute); // Aviso al hilo de ready que ya se puede ejecutar
		sem_post(&s_cont_ready); //Sumo uno al contador de pcbs en ready
		loggear_estado_de_colas();
	}
}


void solicitar_tabla_paginas(PCB_t* proceso){
	log_info(logger, "Solicitando tabla de pag para proceso %d de tamanio %d", proceso->pid, proceso->tamanio);
//	send_crear_tabla(conexion_memoria, proceso->tamanio, proceso->pid);
	op_code cop = CREAR_TABLA;
	send(conexion_memoria, &cop, sizeof(op_code), 0);
	send(conexion_memoria, &proceso->tamanio, sizeof(uint16_t), 0);
	send(conexion_memoria, &proceso->pid, sizeof(uint16_t), 0);
	uint32_t tabla_paginas;
	recv(conexion_memoria, &tabla_paginas, sizeof(uint32_t), 0);
	proceso->tabla_paginas = tabla_paginas;
	log_info(logger, "proceso %d tabla de pag %d", proceso->pid, proceso->tabla_paginas);
}

void loggear_estado_de_colas(){
	pthread_mutex_lock(&mx_log);
	pthread_mutex_lock(&mx_cola_new);
	pthread_mutex_lock(&mx_lista_ready);
	log_info(logger,
			"[NEW/SUSP_READY -> READY] Cola de new: %d | (new -> ready) Cola de ready: %d",
			queue_size(cola_new),
			list_size(lista_ready));
	pthread_mutex_unlock(&mx_log);
	pthread_mutex_unlock(&mx_cola_new);
	pthread_mutex_unlock(&mx_lista_ready);
}

/****Hilo READY -> EXECUTE (FIFO) ***/
void fifo_ready_execute(){
	while(1){
		sem_wait(&s_ready_execute);
		sem_wait(&s_cpu_desocupado); // Para que no ejecute cada vez que un proceso llega a ready
		sem_wait(&s_cont_ready); // Para que no intente ejecutar si la lista de ready esta vacia
		pthread_mutex_lock(&mx_lista_ready);
		PCB_t* proceso = list_remove(lista_ready, 0);
		pthread_mutex_unlock(&mx_lista_ready);
		pthread_mutex_lock(&mx_log);
		log_info(logger,"[READY -> EXECUTE] MANDANDO PROCESO %d A EJECUTAR", proceso->pid);
		pthread_mutex_unlock(&mx_log);
		send_proceso(conexion_cpu_dispatch, proceso, PROCESO);
		pcb_destroy(proceso);
		sem_post(&s_esperar_cpu);
	}
}

void esperar_cpu(){
	while(1){
		sem_wait(&s_esperar_cpu);
		op_code cop;
		if (recv(conexion_cpu_dispatch, &cop, sizeof(op_code), 0) <= 0) {
			pthread_mutex_lock(&mx_log);
			log_error(logger,"DISCONNECT FAILURE!");
			pthread_mutex_unlock(&mx_log);
			exit(-1);
		}
		PCB_t* pcb = pcb_create();
		if (!recv_proceso(conexion_cpu_dispatch, pcb)) {
			pthread_mutex_lock(&mx_log);
			log_error(logger,"Fallo recibiendo PROGRAMA");
			pthread_mutex_unlock(&mx_log);
			return;
		}
		pthread_mutex_lock(&mx_cpu_desocupado);
		cpu_desocupado = true;
		pthread_mutex_unlock(&mx_cpu_desocupado);
		switch (cop) {
			case EXIT:{
				char* key = string_itoa(pcb->pid);
				int socket_pcb = (int) dictionary_get(sockets,key);
				// Hay q avisarle a memoria que finalizo para q borre to-do.
				sem_post(&s_multiprogramacion_actual);
				send(socket_pcb,&cop,sizeof(op_code),0);
				//dictionary_remove_and_destroy(sockets, key, free);
				//free(key);
				log_info(logger, "[EXECUTE -> EXIT] Proceso %d terminado",pcb->pid);

				op_code cop_memoria = ELIMINAR_ESTRUCTURAS;
				send(conexion_memoria, &cop_memoria, sizeof(op_code),0);
				send(conexion_memoria, &pcb->tabla_paginas, sizeof(uint32_t),0);
				send(conexion_memoria, &pcb->pid, sizeof(uint16_t),0);
				//send_eliminar_estructuras(conexion_memoria, pcb->tabla_paginas, pcb->pid);

				pcb_destroy(pcb);
				sem_post(&s_cpu_desocupado);
				sem_post(&s_ready_execute);

				//Por si la interrupcion se mando cuando se estaba procesando la instruccion EXIT
				pthread_mutex_lock(&mx_hay_interrupcion);
				if(hay_interrupcion){
					//pthread_mutex_unlock(&mx_hay_interrupcion);
					sem_post(&s_pcb_desalojado);
				}
				pthread_mutex_unlock(&mx_hay_interrupcion);
				break;
			}
			case INTERRUPTION:
				log_info(logger,"[EXECUTE -> READY] Proceso %d desalojado",pcb->pid);
				pthread_mutex_lock(&mx_lista_ready);
				list_add(lista_ready, pcb);
				pthread_mutex_unlock(&mx_lista_ready);
				sem_post(&s_cont_ready);
				sem_post(&s_pcb_desalojado);
				break;
			case BLOCKED:
				pthread_mutex_lock(&mx_cola_blocked);
				list_add(cola_blocked,pcb);
				pthread_mutex_unlock(&mx_cola_blocked);
				pthread_t hilo_suspendido;
				pthread_create(&hilo_suspendido,NULL,(void*)suspendiendo,pcb);
				pthread_detach(hilo_suspendido);
				log_info(logger, "[EXECUTE -> BLOCKED] Proceso %d bloqueado",pcb->pid);
				sem_post(&s_blocked);
				sem_post(&s_cpu_desocupado);
				sem_post(&s_ready_execute);
				
				//Por si la interrupcion se mando cuando se estaba procesando la instruccion IO
				pthread_mutex_lock(&mx_hay_interrupcion);
				if(hay_interrupcion){
					//pthread_mutex_unlock(&mx_hay_interrupcion);
					sem_post(&s_pcb_desalojado);
				}
				pthread_mutex_unlock(&mx_hay_interrupcion);

				break;
			default:
				log_error(logger, "AAAlgo anduvo mal en el server del kernel\n Cop: %d",cop);
		}
	}
}

void suspendiendo(PCB_t* pcb){
	char* key = string_itoa(pcb->pid);
	pthread_mutex_lock(&mx_iteracion_blocked);
	op_code cop = SUSPENDER_PROCESO;
	int iteracion_actual = (int) dictionary_get(iteracion_blocked, key);
	pthread_mutex_unlock(&mx_iteracion_blocked);
	usleep(tiempo_suspended*1000);
	pthread_mutex_lock(&mx_cola_blocked);
	pthread_mutex_lock(&mx_cola_suspended_blocked);
	int i = pcb_find_index(cola_blocked,pcb->pid);
	if (i == -1){ //No lo encuentra en blocked porque ya finalizo su io.
		pthread_mutex_unlock(&mx_cola_blocked);
		pthread_mutex_unlock(&mx_cola_suspended_blocked);
		pthread_exit(0);
	}
	pthread_mutex_lock(&mx_iteracion_blocked);
	if(iteracion_actual == (int) dictionary_get(iteracion_blocked, key)){
		pthread_mutex_unlock(&mx_iteracion_blocked);
		log_info(logger,"[BLOCKED -> SUSP_BLOCKED] Suspendiendo proceso %d",pcb->pid);

		send(conexion_memoria, &cop, sizeof(op_code),0);
		send(conexion_memoria, &pcb->pid, sizeof(uint16_t),0);
		send(conexion_memoria, &pcb->tabla_paginas, sizeof(uint16_t),0);
		//send_suspender_proceso(conexion_memoria, pcb->pid, pcb->tabla_paginas);

		uint16_t resultado;
		recv(conexion_memoria, &resultado, sizeof(uint16_t), 0);

		list_add(cola_suspended_blocked,pcb->pid);
		pthread_mutex_unlock(&mx_cola_blocked);
		pthread_mutex_unlock(&mx_cola_suspended_blocked);
		//hay que pedir las colas y liberarlas en el mismo orden para evitar deadlocks
		sem_post(&s_multiprogramacion_actual);
		free(key);
		pthread_exit(0);
	}
	pthread_mutex_unlock(&mx_iteracion_blocked);
	pthread_mutex_unlock(&mx_cola_blocked);
	pthread_mutex_unlock(&mx_cola_suspended_blocked);
	free(key);
	pthread_exit(0);
}

void ejecutar_io() {
	while(1) {
		sem_wait(&s_blocked);
		pthread_mutex_lock(&mx_cola_blocked);
		if (list_size(cola_blocked) == 0){
			log_error(logger,"Blocked ejecutó sin un proceso bloqueado");
		}
		PCB_t* proceso = list_get(cola_blocked,0);
		pthread_mutex_unlock(&mx_cola_blocked);
		instruccion_t* inst = list_get(proceso->instrucciones, proceso->pc - 1); //-1 porque ya se incremento el PC
		int32_t tiempo = inst->arg1;
		log_info(logger, "[IO] Proceso %d esperando %d milisegundos", proceso->pid, tiempo);
		usleep(tiempo * 1000);
		pthread_mutex_lock(&mx_cola_blocked);
		list_remove(cola_blocked,0);
		pthread_mutex_unlock(&mx_cola_blocked);
		char* key = string_itoa(proceso->pid);
		pthread_mutex_lock(&mx_iteracion_blocked);
		int iteracion_actual = (int) dictionary_get(iteracion_blocked, key);
		dictionary_put(iteracion_blocked, key,(int *) iteracion_actual + 1);
		free(key);
		pthread_mutex_unlock(&mx_iteracion_blocked);
		if (esta_suspendido(proceso->pid)){
			log_info(logger, "[SUSP_BLOCKED -> SUSP_READY] Proceso %d saliendo de suspended-blocked hacia suspended-ready :)",proceso->pid);
			pthread_mutex_lock(&mx_cola_suspended_ready);
			queue_push(cola_suspended_ready, proceso);
			pthread_mutex_unlock(&mx_cola_suspended_ready);
			sem_post(&s_pasaje_a_ready);
		}
		else{
			log_info(logger, "[BLOCKED -> READY] Proceso %d saliendo de blocked hacia ready :)",proceso->pid);
			pthread_mutex_lock(&mx_lista_ready);
			list_add(lista_ready, proceso);
			pthread_mutex_unlock(&mx_lista_ready);
			sem_post(&s_ready_execute);
			sem_post(&s_cont_ready);
		}
	}
}

bool esta_suspendido(uint16_t pid){
	pthread_mutex_lock(&mx_cola_suspended_blocked);
	t_list_iterator* list_iterator = list_iterator_create(cola_suspended_blocked);
	while(list_iterator_has_next(list_iterator)){
		uint16_t pid_actual = list_iterator_next(list_iterator);
		if (pid == pid_actual){
			list_iterator_remove(list_iterator); //Remueve de la lista de suspendidos al elemento
			list_iterator_destroy(list_iterator);
			pthread_mutex_unlock(&mx_cola_suspended_blocked);
			return true;
		}
	}
	list_iterator_destroy(list_iterator);
	pthread_mutex_unlock(&mx_cola_suspended_blocked);
	return false;
}

void printear_estado_semaforos(){
	int sem_valor;
	sem_getvalue (&s_pasaje_a_ready, &sem_valor);
	log_info(logger, "[DEBUG] Pasaje a ready: %d", sem_valor);
	sem_getvalue (&s_ready_execute, &sem_valor);
	log_info(logger,"[DEBUG] Ready a execute: %d", sem_valor);
	sem_getvalue (&s_cont_ready, &sem_valor);
	log_info(logger,"[DEBUG] Contador de ready: %d", sem_valor);
	sem_getvalue (&s_blocked, &sem_valor);
	log_info(logger,"[DEBUG] Blocked: %d", sem_valor);
	sem_getvalue (&s_suspended_ready, &sem_valor);
	log_info(logger,"[DEBUG] Suspended -> ready: %d", sem_valor);
	sem_getvalue (&s_multiprogramacion_actual, &sem_valor);
	log_info(logger,"[DEBUG] Multiprogramacion actual: %d", sem_valor);
	sem_getvalue (&s_pcb_desalojado, &sem_valor);
	log_info(logger,"[DEBUG] PCB desalojado: %d", sem_valor);
	sem_getvalue (&s_esperar_cpu, &sem_valor);
	log_info(logger,"[DEBUG] Esperar CPU: %d", sem_valor);
}

/****Hilo READY -> EXECUTE (SRT) ***/
void srt_ready_execute(){
	while(1){
		int sem_valor;
		sem_getvalue (&s_ready_execute, &sem_valor);
		if(sem_valor == 1) //Se le dio post mientras no se habia terminado de mandar a otro a ejecutar
			sem_wait(&s_ready_execute);
		sem_wait(&s_ready_execute);
		sem_wait(&s_cont_ready); // Para que no intente ejecutar si la lista de ready esta vacia
		pthread_mutex_lock(&mx_cpu_desocupado);
		if(!cpu_desocupado){
			pthread_mutex_unlock(&mx_cpu_desocupado);
			pthread_mutex_lock(&mx_hay_interrupcion);
			hay_interrupcion =  true;
			pthread_mutex_unlock(&mx_hay_interrupcion);
			desalojar_cpu();
			sem_wait(&s_pcb_desalojado);
			pthread_mutex_lock(&mx_hay_interrupcion);
			hay_interrupcion =  false;
			pthread_mutex_unlock(&mx_hay_interrupcion);
		}
		else
			pthread_mutex_unlock(&mx_cpu_desocupado);
		PCB_t* proceso = seleccionar_proceso_srt();
		pthread_mutex_lock(&mx_log);
		log_info(logger,"[READY -> EXECUTE] Mandando proceso %d a ejecutar",proceso->pid);
		pthread_mutex_unlock(&mx_log);
		pthread_mutex_lock(&mx_cpu_desocupado);
		cpu_desocupado = false;
		pthread_mutex_unlock(&mx_cpu_desocupado);
		send_proceso(conexion_cpu_dispatch, proceso, PROCESO);
		pcb_destroy(proceso);
		sem_post(&s_esperar_cpu);
	}
}

PCB_t* seleccionar_proceso_srt(){
	// hago un mutex lock y unlock para tod.o el proceso entero para que no me agreguen
	// procesos a la mitad del procedimiento
	pthread_mutex_lock(&mx_lista_ready);
	PCB_t* primer_pcb = list_get(lista_ready,0);
	double raf_min = primer_pcb->est_rafaga;
	int index_pcb = 0;
	for (int i = 0; i < list_size(lista_ready); i++){
		PCB_t* pcb = list_get(lista_ready,i);
		double est_raf = pcb->est_rafaga;
		if(est_raf < raf_min){ //Asumimos que en caso de empate es FIFO.
			raf_min = est_raf; //Solo se reemplaza si la nueva es menor.
			index_pcb = i;
		}
	}
	PCB_t* pcb = list_remove(lista_ready, index_pcb);
	pthread_mutex_unlock(&mx_lista_ready);
	return pcb;
}

void desalojar_cpu(){
	op_code codigo = INTERRUPTION;
	log_info(logger, "[INTERRUPT] Mandando interrupcion");
	send(conexion_cpu_interrupt, &codigo, sizeof(op_code), 0);
}
