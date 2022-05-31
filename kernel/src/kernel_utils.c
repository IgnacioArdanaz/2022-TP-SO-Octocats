#include "kernel_utils.h"

pthread_mutex_t mx_multip_actual = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_pid_sig = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cpu_desocupado = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cola_new = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_lista_ready = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_socket = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_log = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cola_blocked = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cola_suspended_blocked = PTHREAD_MUTEX_INITIALIZER;


sem_t s_new_ready, s_ready_execute, s_cont_ready, s_cpu_desocupado, s_blocked, s_suspended_ready, s_multiprogramacion_actual;

t_dictionary* sockets;
t_dictionary* hilos_suspensores; //indexado por pid
t_queue* cola_new;
t_list* lista_ready;
t_list* cola_blocked;
t_list* cola_suspended_blocked;


//safe_log* safe_logger;

int pid_sig, estimacion_inicial, grado_multiprogramacion,
	multiprogramacion_actual, conexion_cpu_dispatch, conexion_cpu_interrupt, tiempo_suspended;
char *algoritmo_config, *ip_cpu, *puerto_cpu_dispatch, *puerto_cpu_interrupt;

void inicializar_kernel(){
	logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);
	config = config_create("kernel.config");

	cola_new = queue_create();
	cola_blocked = list_create();
	lista_ready = list_create();
	cola_suspended_blocked = list_create();

	hilos_suspensores = dictionary_create();
	sockets = dictionary_create();

	pid_sig = 1;

	grado_multiprogramacion = config_get_int_value(config,"GRADO_MULTIPROGRAMACION");
	estimacion_inicial = config_get_double_value(config,"ESTIMACION_INICIAL");
	ip_cpu = config_get_string_value(config,"IP_CPU");
	puerto_cpu_dispatch = config_get_string_value(config,"PUERTO_CPU_DISPATCH");
	puerto_cpu_interrupt = config_get_string_value(config,"PUERTO_CPU_INTERRUPT");
	algoritmo_config = config_get_string_value(config,"ALGORITMO_PLANIFICACION");
	tiempo_suspended = config_get_int_value(config,"TIEMPO_MAXIMO_BLOQUEADO");

	conexion_cpu_dispatch = crear_conexion(logger, "CPU DISPATCH", ip_cpu, puerto_cpu_dispatch);
	conexion_cpu_interrupt= crear_conexion(logger, "CPU INTERRUPT", ip_cpu, puerto_cpu_interrupt);

	sem_init(&s_new_ready, 0, 0);
	sem_init(&s_cont_ready, 0, 0); // Incrementa al sumar un proceso a ready y decrementa al ejecutarlo
	sem_init(&s_blocked, 0, 0);
	sem_init(&s_suspended_ready, 0, 0);
	sem_init(&s_cpu_desocupado, 0, 1);
	sem_init(&s_multiprogramacion_actual, 0, grado_multiprogramacion);
	sem_init(&s_ready_execute, 0, 0);

	pthread_t hilo_pasaje_new_ready;
	pthread_create(&hilo_pasaje_new_ready,NULL,(void*)pasaje_new_ready,NULL);
	pthread_detach(hilo_pasaje_new_ready);

	pthread_t hilo_blocked;
	pthread_create(&hilo_blocked,NULL,(void*)ejecutar_io,NULL);
	pthread_detach(hilo_blocked);

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
	printf("Kernel esperando un nuevo cliente \n");
	int cliente_socket = cliente_socket = esperar_cliente(logger, name, server_socket);
	printf("Socket del cliente recibido en kernel.\n ");
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
	int32_t resultError = -1;
	free(argumentos);
	op_code cop;
	while (cliente_socket != -1) {
		if (recv(cliente_socket, &cop, sizeof(op_code), 0) <= 0) {
			pthread_mutex_lock(&mx_log);
			log_error(logger,"DISCONNECT FAILURE!");
			pthread_mutex_unlock(&mx_log);
			send(cliente_socket, &resultError, sizeof(int32_t), 0);
			return;
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
				dictionary_put(sockets, key, cliente_socket);
				pthread_mutex_unlock(&mx_socket);
				sem_post(&s_new_ready); //Avisa al hilo planificador de pasaje de new a ready que debe ejecutarse.


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

/****Hilo NEW -> READY ***/
void pasaje_new_ready(){
	while(1){
		sem_wait(&s_new_ready);
		//AGREGAR QUE SI HAY PCBS EN READY SUSPENDED TIENEN PRIORIDAD DE PASO
		sem_wait(&s_multiprogramacion_actual);
		pthread_mutex_lock(&mx_cola_new);
		PCB_t* proceso = queue_pop(cola_new);
		printf("PROCESO %d POPEADO COLA NEW\n", proceso->pid);
		pthread_mutex_unlock(&mx_cola_new);
		pthread_mutex_lock(&mx_lista_ready);
		list_add(lista_ready,proceso);
		pthread_mutex_unlock(&mx_lista_ready);
		sem_post(&s_cont_ready); //Sumo uno al contador de ready
//		pthread_mutex_lock(&mx_multip_actual);
//		sem_post(&s_multiprogramacion_actual);
//		pthread_mutex_unlock(&mx_multip_actual);
		sem_post(&s_ready_execute);
		loggear_estado_de_colas();
		//imprimir_lista_ready();
	}

}

void loggear_estado_de_colas(){
	pthread_mutex_lock(&mx_log);
	log_info(logger,
			"(new -> ready) Cola de new: %d | (new -> ready) Cola de ready: %d",
			queue_size(cola_new),
			list_size(lista_ready));
	pthread_mutex_unlock(&mx_log);
}

/****Hilo READY -> EXECUTE (FIFO) ***/
void fifo_ready_execute(){
	while(1){
		sem_wait(&s_ready_execute);
		sem_wait(&s_cpu_desocupado);
		// Para que no ejecute cada vez que un proceso pasa de new a ready
		sem_wait(&s_cont_ready); // Para que no intente ejecutar si la lista de ready esta vacia
		pthread_mutex_lock(&mx_lista_ready);
		PCB_t* proceso = list_remove(lista_ready, 0);
		pthread_mutex_unlock(&mx_lista_ready);
		pthread_mutex_lock(&mx_log);
		log_info(logger,"Mandando proceso %d a ejecutar",proceso->pid);
		pthread_mutex_unlock(&mx_log);
		pthread_mutex_lock(&mx_cpu_desocupado);
		pthread_mutex_unlock(&mx_cpu_desocupado);
		send_proceso(conexion_cpu_dispatch, proceso, PROCESO);
		pcb_destroy(proceso);
		esperar_cpu();
	}
}

void esperar_cpu(){
	op_code cop;
	if (recv(conexion_cpu_dispatch, &cop, sizeof(op_code), 0) <= 0) {
		pthread_mutex_lock(&mx_log);
		log_error(logger,"DISCONNECT FAILURE!");
		pthread_mutex_unlock(&mx_log);
		return;
	}
	PCB_t* pcb = pcb_create();
	if (!recv_proceso(conexion_cpu_dispatch, pcb)) {
		pthread_mutex_lock(&mx_log);
		log_error(logger,"Fallo recibiendo PROGRAMA");
		pthread_mutex_unlock(&mx_log);
		return;
	}
	switch (cop) {
		case EXIT:{
			int socket_pcb = (int) dictionary_get(sockets,string_itoa(pcb->pid));
			printf("Socket: %d", socket_pcb);
			// Hay q avisarle a memoria que finalizo para q borre todo.
			//int el_socket = 0;
			sem_post(&s_multiprogramacion_actual);
			send(socket_pcb,&cop,sizeof(op_code),0);
			log_info(logger, "Proceso %d terminado :) siiiii",pcb->pid);
			pcb_destroy(pcb);
			break;
		}
		case INTERRUPTION:
			log_info(logger,"Proceso %d desalojado :( lo siento",pcb->pid);
			list_add(lista_ready,pcb);
			break;
		case BLOCKED:
			//bloqueamos el proceso
			pthread_mutex_lock(&mx_cola_blocked);
			list_add(cola_blocked,pcb);
			pthread_mutex_unlock(&mx_cola_blocked);
//			pthread_t hilo_suspendido;
//			pthread_create(&hilo_suspendido,NULL,(void*)suspendiendo,pcb);
			log_info(logger, "Proceso %d bloqueado :( mal ahi pa",pcb->pid);
			sem_post(&s_blocked);
			break;
		default:
			log_error(logger, "AAAlgo anduvo mal en el server del kernel\n Cop: %d",cop);
	}
	sem_post(&s_cpu_desocupado);
	sem_post(&s_ready_execute);
}

// ESTA OPCION NO SIRVE
// QUIZAS DEBAMOS MANDARLE UNA SEÑAL AL THREAD CUANDO
void suspendiendo(PCB_t* pcb){
	usleep(tiempo_suspended*1000);
	pthread_mutex_lock(&mx_cola_blocked);
	pthread_mutex_lock(&mx_cola_suspended_blocked);
	int i = pcb_find_index(cola_blocked,pcb->pid);
	printf("Index del suspended: %d",i);
	if (i == -1)
		pthread_mutex_unlock(&mx_cola_blocked);
		pthread_mutex_unlock(&mx_cola_suspended_blocked);
		return;
	log_info(logger,"Suspendiendo proceso %d que bajon :(",pcb->pid);
	list_remove(cola_blocked,i);
	list_add(cola_suspended_blocked,pcb);
	pthread_mutex_unlock(&mx_cola_blocked);
	pthread_mutex_unlock(&mx_cola_suspended_blocked);
	//hay que pedir las colas y liberarlas en el mismo orden para evitar deadlocks
	sem_post(&s_multiprogramacion_actual);
}

void ejecutar_io() {
	while(1) {
		sem_wait(&s_blocked);
		pthread_mutex_lock(&mx_cola_blocked);
		PCB_t* proceso = list_remove(cola_blocked,0);
		pthread_mutex_unlock(&mx_cola_blocked);
		instruccion_t* inst = list_get(proceso->instrucciones, proceso->pc - 1);
		int32_t tiempo = inst->arg1; // Se hace - 1 porque ya se incremento el PC
		log_info(logger, "[IO] Proceso %d esperando %d milisegundos", proceso->pid, tiempo);
		usleep(tiempo * 1000);
		log_info(logger, "[IO] Proceso %d saliendo de blocked :)",proceso->pid);
		pthread_mutex_lock(&mx_lista_ready);
		list_add(lista_ready, proceso);
		pthread_mutex_unlock(&mx_lista_ready);
		sem_post(&s_ready_execute);
		sem_post(&s_cont_ready);
		}
}

/****Hilo READY -> EXECUTE (SRT) ***/
void srt_ready_execute(){
	while(1){
//		sem_wait(&s_algorithm_ready_execute);
//		pthread_mutex_lock(&mx_cola_new);
//		//AGREGAR QUE SI HAY PCBS EN READY SUSPENDED TIENEN PRIORIDAD DE PASO
//		if(multiprogramacion_actual < grado_multiprogramacion){
//			PCB_t* p = queue_pop(cola_new);
//			list_add(lista_ready,p);
//			pthread_mutex_lock(&mx_multip_actual);
//			multiprogramacion_actual++;
//			pthread_mutex_unlock(&mx_multip_actual);
//		}
//		pthread_mutex_unlock(&mx_cola_new);
	}

}

//Este no va pero se puede usar la logica para hacer srt_ready_execute
//PCB_t* sjf(){
//
//	PCB_t* primer_pcb = list_get(lista_ready,0);
//	double raf_min = primer_pcb->est_rafaga;
//	int index_pcb = 0;
//	int i;
//	for (i = 1;i<list_size(lista_ready);i++){
//		PCB_t* pcb = list_get(lista_ready,i);
//		double est_raf = pcb->est_rafaga;
//		//si el sjf luego sigue con FIFO, entonces tiene que ser >
//		//si el sjf luego sigue con LIFO, entonces tiene que ser >=
//		if(raf_min >= est_raf){
//			raf_min = est_raf;
//			index_pcb = i;
//		}
//	}
//	return list_remove(lista_ready,index_pcb);
//
//}
