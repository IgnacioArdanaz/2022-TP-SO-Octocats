#include "kernel_utils.h"

pthread_mutex_t mx_multip_actual = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_pid_sig = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cola_new = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_lista_ready = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_socket = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_log = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cola_blocked = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cola_suspended_blocked = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cola_suspended_ready = PTHREAD_MUTEX_INITIALIZER;

sem_t s_pasaje_a_ready, s_ready_execute, s_cont_ready, s_cpu_desocupado, s_blocked,
	s_suspended_ready, s_multiprogramacion_actual, s_pcb_desalojado;

t_dictionary* sockets;
t_queue* cola_new;
t_list* lista_ready;
t_list* cola_blocked;
t_list* cola_suspended_blocked;
t_queue* cola_suspended_ready;

bool cpu_desocupado;

// CHEQUEAR SI ALGUNO DE ESTAS VARIABLES PUEDEN SER LOCALES DE LA FUNCION INICIALIZAR
// ES PREFERIBLE QUE SEAN LOCALES A QUE SEAN GLOBALES
int pid_sig, grado_multiprogramacion, multiprogramacion_actual,
	conexion_cpu_dispatch, conexion_cpu_interrupt, tiempo_suspended, conexion_memoria;

double estimacion_inicial;


void inicializar_kernel(){

	logger = log_create("kernel.log", "KERNEL", 1, LOG_LEVEL_INFO);
	config = config_create("kernel.config");

	cola_new = queue_create();
	cola_blocked = list_create();
	lista_ready = list_create();
	cola_suspended_blocked = list_create();
	cola_suspended_ready = queue_create();
	sockets = dictionary_create();

	pid_sig = 1;
	cpu_desocupado = true;

	grado_multiprogramacion = config_get_int_value(config,"GRADO_MULTIPROGRAMACION");
	estimacion_inicial = config_get_double_value(config,"ESTIMACION_INICIAL");
	char* algoritmo_config = config_get_string_value(config,"ALGORITMO_PLANIFICACION");
	tiempo_suspended = config_get_int_value(config,"TIEMPO_MAXIMO_BLOQUEADO");
	double alfa = config_get_double_value(config,"ALFA");

	//conectando con MEMORIA
//	char* ip_memoria = config_get_string_value(config,"IP_MEMORIA");
//	char* puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");
//	conexion_memoria = crear_conexion(logger, "MEMORIA", ip_memoria, puerto_memoria);
//
//	if (conexion_memoria == 0){
//		log_error(logger,"Error al intentar conectarse a memoria :-(");
//		exit(EXIT_FAILURE);
//	}


	//conectando con CPU
	char* ip_cpu = config_get_string_value(config,"IP_CPU");
	char* puerto_cpu_dispatch = config_get_string_value(config,"PUERTO_CPU_DISPATCH");
	char* puerto_cpu_interrupt = config_get_string_value(config,"PUERTO_CPU_INTERRUPT");
	conexion_cpu_dispatch = crear_conexion(logger, "CPU DISPATCH", ip_cpu, puerto_cpu_dispatch);
	conexion_cpu_interrupt= crear_conexion(logger, "CPU INTERRUPT", ip_cpu, puerto_cpu_interrupt);
	send(conexion_cpu_dispatch,&alfa,sizeof(alfa),0);


	sem_init(&s_pasaje_a_ready, 0, 0);
	sem_init(&s_cont_ready, 0, 0); // Incrementa al sumar un proceso a ready y decrementa al ejecutarlo
	sem_init(&s_blocked, 0, 0);
	sem_init(&s_suspended_ready, 0, 0);
	sem_init(&s_cpu_desocupado, 0, 1);
	sem_init(&s_multiprogramacion_actual, 0, grado_multiprogramacion);
	sem_init(&s_ready_execute, 0, 0);
	sem_init(&s_pcb_desalojado, 0, 0);

	pthread_t hilo_pasaje_a_ready;
	pthread_create(&hilo_pasaje_a_ready,NULL,(void*)pasaje_a_ready,NULL);
	pthread_detach(hilo_pasaje_a_ready);

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
	log_info(logger,"Kernel esperando un nuevo cliente");
	int cliente_socket = cliente_socket = esperar_cliente(logger, name, server_socket);
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
				sem_post(&s_pasaje_a_ready); //Avisa al hilo planificador de pasaje a ready que debe ejecutarse.

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
		if(queue_is_empty(cola_suspended_ready)){ //Si no hay suspendidos, agarro uno de new
			pthread_mutex_lock(&mx_cola_new);
			proceso = queue_pop(cola_new);
			log_info(logger,"PROCESO %d POPEADO COLA NEW", proceso->pid);
			pthread_mutex_unlock(&mx_cola_new);
		}
		else{
			pthread_mutex_lock(&mx_cola_suspended_ready);
			proceso = queue_pop(cola_suspended_ready);
			log_info(logger,"PROCESO %d POPEADO COLA SUSPENDED READY", proceso->pid);
			pthread_mutex_unlock(&mx_cola_suspended_ready);
		}
		pthread_mutex_lock(&mx_lista_ready);
		list_add(lista_ready, proceso);
		pthread_mutex_unlock(&mx_lista_ready);
		sem_post(&s_cont_ready); //Sumo uno al contador de pcbs en ready
		sem_post(&s_ready_execute); // Aviso al hilo de ready que ya se puede ejecutar
		loggear_estado_de_colas();
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
		sem_wait(&s_cpu_desocupado); // Para que no ejecute cada vez que un proceso llega a ready
		sem_wait(&s_cont_ready); // Para que no intente ejecutar si la lista de ready esta vacia
		pthread_mutex_lock(&mx_lista_ready);
		PCB_t* proceso = list_remove(lista_ready, 0);
		pthread_mutex_unlock(&mx_lista_ready);
		pthread_mutex_lock(&mx_log);
		log_info(logger,"Mandando proceso %d a ejecutar",proceso->pid);
		pthread_mutex_unlock(&mx_log);
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
			// Hay q avisarle a memoria que finalizo para q borre todo.
			sem_post(&s_multiprogramacion_actual);
			send(socket_pcb,&cop,sizeof(op_code),0);
			log_info(logger, "Proceso %d terminado",pcb->pid);
			pcb_destroy(pcb);
			break;
		}
		case INTERRUPTION:
			log_info(logger,"Proceso %d desalojado",pcb->pid);
			pthread_mutex_lock(&mx_lista_ready);
			list_add(lista_ready, pcb);
			pthread_mutex_unlock(&mx_lista_ready);
			sem_post(&s_pcb_desalojado);
			return; //Porque no quiero que haga los sem_post de despues del switch
		case BLOCKED:
			pthread_mutex_lock(&mx_cola_blocked);
			list_add(cola_blocked,pcb);
			pthread_mutex_unlock(&mx_cola_blocked);
			pthread_t hilo_suspendido;
			pthread_create(&hilo_suspendido,NULL,(void*)suspendiendo,pcb);
			pthread_detach(hilo_suspendido);
			log_info(logger, "Proceso %d bloqueado",pcb->pid);
			sem_post(&s_blocked);
			break;
		default:
			log_error(logger, "AAAlgo anduvo mal en el server del kernel\n Cop: %d",cop);
	}
	sem_post(&s_cpu_desocupado);
	cpu_desocupado = true;
	sem_post(&s_ready_execute);
}

void suspendiendo(PCB_t* pcb){
	usleep(tiempo_suspended*1000);
	pthread_mutex_lock(&mx_cola_blocked);
	pthread_mutex_lock(&mx_cola_suspended_blocked);
	int i = pcb_find_index(cola_blocked,pcb->pid);
	if (i == -1){ //No lo encuentra en blocked porque ya finalizo su io.
		pthread_mutex_unlock(&mx_cola_blocked);
		pthread_mutex_unlock(&mx_cola_suspended_blocked);
		pthread_exit(0);
	}
	log_info(logger,"Suspendiendo proceso %d",pcb->pid);
	list_add(cola_suspended_blocked,pcb->pid);
	pthread_mutex_unlock(&mx_cola_blocked);
	pthread_mutex_unlock(&mx_cola_suspended_blocked);
	//hay que pedir las colas y liberarlas en el mismo orden para evitar deadlocks
	sem_post(&s_multiprogramacion_actual);
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
		if (esta_suspendido(proceso->pid)){
			log_info(logger, "[IO] Proceso %d saliendo de blocked hacia suspended-ready :)",proceso->pid);
			pthread_mutex_lock(&mx_cola_suspended_ready);
			queue_push(cola_suspended_ready, proceso);
			pthread_mutex_unlock(&mx_cola_suspended_ready);
			sem_post(&s_pasaje_a_ready);
		}
		else{
			log_info(logger, "[IO] Proceso %d saliendo de blocked hacia ready :)",proceso->pid);
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

/****Hilo READY -> EXECUTE (SRT) ***/
void srt_ready_execute(){
	while(1){
		sem_wait(&s_ready_execute);
		sem_wait(&s_cont_ready); // Para que no intente ejecutar si la lista de ready esta vacia
		if(!cpu_desocupado){
			desalojar_cpu();
		}
		pthread_mutex_lock(&mx_lista_ready);
		PCB_t* proceso = seleccionar_proceso_srt(); // mx porque la funcion usa a la cola de ready
		pthread_mutex_unlock(&mx_lista_ready);
		pthread_mutex_lock(&mx_log);
		log_info(logger,"Mandando proceso %d a ejecutar",proceso->pid);
		pthread_mutex_unlock(&mx_log);
		cpu_desocupado = false;
		send_proceso(conexion_cpu_dispatch, proceso, PROCESO);
		pcb_destroy(proceso);
		esperar_cpu();
	}
}


PCB_t* seleccionar_proceso_srt(){
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
	return list_remove(lista_ready, index_pcb);
}

void desalojar_cpu(){
	op_code codigo = INTERRUPTION;
	send(conexion_cpu_interrupt, &codigo, sizeof(op_code), 0);
	sem_wait(&s_pcb_desalojado);
}
