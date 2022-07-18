#include "cpu_main.h"

pthread_mutex_t mx_hay_interrupcion = PTHREAD_MUTEX_INITIALIZER;

t_log* logger;
t_config* config;

//VER SI ALGUNO DE ESTOS PUEDE SER LOCAL
int espera, server_cpu_dispatch, server_cpu_interrupt, cliente_socket_interrupt,
	cliente_socket_dispatch, operando_copy, entradas_tlb, conexion_memoria;
char* reemplazo_tlb;
bool hay_interrupcion;
double alfa;
uint64_t rafaga_real;
uint16_t tam_pagina;
uint16_t cant_ent_paginas;
t_list* tlb;
uint16_t pid_actual=0;

int main(void) {

	inicializar_cpu();
	while(1){
		PCB_t* pcb = pcb_create();
		op_code cop;
		recv(cliente_socket_dispatch, &cop, sizeof(op_code), 0);
		if(!recv_proceso(cliente_socket_dispatch,pcb)){
			log_error(logger, "DISCONNECT FAILURE EN KERNEL!!");
			exit(-1);
		}
		
		log_info(logger,"Proceso %d -> program counter %d - est %f", pcb->pid, pcb->pc, pcb->est_rafaga);
		limpiar_tlb();
		pid_actual = pcb->pid;
		struct timespec start;
		clock_gettime(CLOCK_REALTIME,&start);
		op_code estado = iniciar_ciclo_instruccion(pcb);
		struct timespec end;
		clock_gettime(CLOCK_REALTIME, &end);
		rafaga_real = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
		calculo_estimacion(pcb, estado);

		log_info(logger,"Program counter %d (despues de ejecutar)",pcb->pc);
		log_info(logger,"==============================================================");
		pthread_mutex_lock(&mx_hay_interrupcion);
		hay_interrupcion = false;
		pthread_mutex_unlock(&mx_hay_interrupcion);
		send_proceso(cliente_socket_dispatch,pcb,estado);
		pcb_destroy(pcb);
	}

	apagar_cpu();

	return EXIT_SUCCESS;
}

void calculo_estimacion(PCB_t *pcb, op_code estado){
	log_info(logger, "Rafaga real %" PRIu64 , rafaga_real);

	if(estado == INTERRUPTION){
		pcb->est_rafaga = pcb->est_rafaga - (double)rafaga_real;
	} else {
		pcb->est_rafaga = pcb->est_rafaga *alfa + (double)rafaga_real*(1-alfa); //Calculo estimacion rafaga siguiente
	}
	log_info(logger, "Proceso %d estimacion rafaga siguiente %f",pcb->pid, pcb->est_rafaga);
}

void inicializar_cpu(){
	logger = log_create("cpu.log", "cpu", 1, LOG_LEVEL_INFO);
	config = config_create("../cpu.config");
	espera = config_get_int_value(config, "RETARDO_NOOP");
	entradas_tlb = config_get_int_value(config, "ENTRADAS_TLB");
	reemplazo_tlb = config_get_string_value(config, "REEMPLAZO_TLB");
	log_info(logger,"Algoritmo de reemplazo configurado: %s", reemplazo_tlb);
	hay_interrupcion = false;
	tlb=list_create();

	inicializar_tlb();

	/*for(int i=0;i< entradas_tlb;i++){
			TLB_t *tlb_aux2 = malloc(sizeof(TLB_t));
			tlb_aux2 = list_get(tlb,i);
			log_info(logger,"Numero pagina: %d", tlb_aux2->pagina);
			log_info(logger,"Numero pagina: %d", tlb_aux2->marco);
			free(tlb_aux2);
		}*/

	operando_copy = 0;
	t_config* config_ips = config_create("../../ips.config");
	char* ip = config_get_string_value(config_ips,"IP_CPU");
	char* ip_memoria = config_get_string_value(config_ips,"IP_MEMORIA");

	//conexion con MEMORIA
	//char* ip_memoria = config_get_string_value(config,"IP_MEMORIA");
	char* puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");
	conexion_memoria = crear_conexion(logger, "MEMORIA", ip_memoria, puerto_memoria);

	if (conexion_memoria == 0){
		log_error(logger,"Error al intentar conectarse a memoria :-(");
		exit(EXIT_FAILURE);
	}

	recv(conexion_memoria, &cant_ent_paginas, sizeof(uint16_t), 0);
	recv(conexion_memoria, &tam_pagina, sizeof(uint16_t), 0);
//	recv_datos_necesarios(conexion_memoria, &cant_ent_paginas, &tam_pagina);
	log_info(logger, "RECIBIDO: tam_pagina=%d - cant_ent_paginas=%d", tam_pagina, cant_ent_paginas);


	//char* ip = config_get_string_value(config, "IP");
	char* puerto_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	char* puerto_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
	server_cpu_dispatch = iniciar_servidor(logger, "CPU_DISPATCH", ip, puerto_dispatch);
	server_cpu_interrupt = iniciar_servidor(logger, "CPU_INTERRUPT", ip, puerto_interrupt);
	cliente_socket_dispatch = esperar_cliente(logger, "CPU_DISPATCH",server_cpu_dispatch);
	cliente_socket_interrupt = esperar_cliente(logger, "CPU_INTERRUPT", server_cpu_interrupt);
	recv(cliente_socket_dispatch, &alfa, sizeof(double), 0);
	log_info(logger,"Valor alfa %.2f",alfa);
	config_destroy(config_ips);
	pthread_t hilo_interrupciones;
	pthread_create(&hilo_interrupciones,NULL,(void*)interrupcion,NULL);
	pthread_detach(hilo_interrupciones);

}


void inicializar_tlb(){

	for(int i=0;i< entradas_tlb;i++){
		TLB_t *tlb_aux = crear_entrada_tlb(-1,-1);
		list_add(tlb, tlb_aux);

	}

}


void limpiar_tlb(){
	list_clean_and_destroy_elements(tlb,free);
	inicializar_tlb(tlb);
}

TLB_t *crear_entrada_tlb(int32_t pagina, int32_t marco){
	TLB_t *tlb_entrada= malloc(sizeof(TLB_t));

	tlb_entrada->marco = marco;
	tlb_entrada->pagina = pagina;
	tlb_entrada->ultima_referencia = clock();
	return tlb_entrada;
}

void apagar_cpu(){
	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(&server_cpu_dispatch);
	liberar_conexion(&server_cpu_interrupt);
}

op_code iniciar_ciclo_instruccion(PCB_t* pcb){
	op_code estado = CONTINUE;
	while (estado == CONTINUE){ // Solo sale si hay una interrupcion, un pedido de I/O, o fin de ejecucion
		instruccion_t* instruccion_ejecutar = fetch(pcb->instrucciones, pcb->pc);

		if(decode(instruccion_ejecutar)){
			log_info(logger,"Aca busco el operando");
			instruccion_ejecutar->arg2  = fetch_operands(instruccion_ejecutar->arg2,pcb->tabla_paginas);

		}
		estado = execute(instruccion_ejecutar,pcb->tabla_paginas);
		if(estado == CONTINUE){
			estado = check_interrupt();
		}
		pcb->pc++;
	}
	return estado;
}

instruccion_t* fetch(t_list* instrucciones, uint32_t pc){
	return list_get(instrucciones,pc);
}

int check_interrupt(){
	pthread_mutex_lock(&mx_hay_interrupcion);
	if (hay_interrupcion){
		hay_interrupcion = false;
		pthread_mutex_unlock(&mx_hay_interrupcion);
		return INTERRUPTION;
	}
	pthread_mutex_unlock(&mx_hay_interrupcion);
	return CONTINUE;
}

int fetch_operands(int direccion, uint32_t tabla){
	log_info(logger,"Fetch operands...");
	return ejecutarRead(direccion,tabla);
}


int decode(instruccion_t* instruccion_ejecutar ){
	return instruccion_ejecutar->operacion == 'C';
}



int ejecutarRead(uint32_t dir_logica,uint32_t tabla_paginas){
	marco_t dir_fisica;
	uint32_t valor = 8;
	dir_fisica = traducir_direccion(dir_logica,tabla_paginas);
	log_info(logger, "Ejecutando READ para pagina %d, marco %d y desplazamiento %d",
			dir_logica / tam_pagina, dir_fisica.marco, dir_fisica.desplazamiento);
//	send_read(conexion_memoria, dir_fisica.marco, dir_fisica.desplazamiento);

	op_code cop = READ;
	send(conexion_memoria, &cop, sizeof(op_code),0);
	send(conexion_memoria, &dir_fisica.marco, sizeof(uint32_t),0);
	send(conexion_memoria, &dir_fisica.desplazamiento, sizeof(uint16_t),0);

	recv(conexion_memoria, &valor, sizeof(uint32_t), 0);
	log_info(logger, "Dato leido = %d", valor);
	return valor;
}



void ejecutarWrite(uint32_t dir_logica,uint32_t valor,uint32_t tabla_paginas ){
	marco_t dir_fisica;
	op_code resultado;
	dir_fisica = traducir_direccion(dir_logica, tabla_paginas);

	op_code cop = WRITE;
	send(conexion_memoria, &cop, sizeof(op_code),0);
	send(conexion_memoria, &dir_fisica.marco, sizeof(uint32_t),0);
	send(conexion_memoria, &dir_fisica.desplazamiento, sizeof(uint32_t),0);
	send(conexion_memoria, &valor, sizeof(uint32_t),0);

	log_info(logger, "Ejecutando WRITE: pagina %d, marco %d,  desp %d, valor: %d",
			dir_logica / tam_pagina, dir_fisica.marco, dir_fisica.desplazamiento, valor);
//	send_write(conexion_memoria,dir_fisica.marco,dir_fisica.desplazamiento,valor);
	recv(conexion_memoria, &resultado, sizeof(op_code), 0);
	//recv_verificacion
}

marco_t  traducir_direccion(uint32_t dir_logica,uint32_t tabla_paginas_1){
	marco_t dire_fisica;
	uint32_t nro_marco = 145;
	uint32_t numero_pagina = floor( (double)dir_logica / (double)tam_pagina );
	//Aca hay que fijarse si esta en la tlb
	nro_marco = presente_en_tlb(numero_pagina);

	if(nro_marco==-1){
		log_info(logger, "TLB MISS (pagina %d)", numero_pagina);
		uint32_t entrada_tabla_1 = floor((double)numero_pagina/(double)cant_ent_paginas);
		uint32_t entrada_tabla_2 = numero_pagina % cant_ent_paginas;
		int32_t nro_tabla_2do_nivel = 237;


//		send_solicitud_nro_tabla_2do_nivel(conexion_memoria, pid_actual, tabla_paginas_1,  entrada_tabla_1);
		op_code cop = SOLICITUD_NRO_TABLA_2DO_NIVEL;
		send(conexion_memoria, &cop, sizeof(op_code),0);
		send(conexion_memoria, &pid_actual, sizeof(uint16_t),0);
		send(conexion_memoria, &tabla_paginas_1, sizeof(uint32_t),0);
		send(conexion_memoria, &entrada_tabla_1, sizeof(uint32_t),0);
		recv(conexion_memoria, &nro_tabla_2do_nivel, sizeof(int32_t), 0);
		log_info(logger, "Recibida tabla pag 2do nivel numero: %d", nro_tabla_2do_nivel);

//		send_solicitud_nro_marco(conexion_memoria, pid_actual, nro_tabla_2do_nivel,  entrada_tabla_2,entrada_tabla_1);
		cop = SOLICITUD_NRO_MARCO;
		send(conexion_memoria, &cop, sizeof(op_code),0);
		send(conexion_memoria, &pid_actual, sizeof(uint16_t),0);
		send(conexion_memoria, &nro_tabla_2do_nivel, sizeof(int32_t),0);
		send(conexion_memoria, &entrada_tabla_2, sizeof(uint32_t),0);
		send(conexion_memoria, &entrada_tabla_1, sizeof(uint32_t),0);
		recv(conexion_memoria, &nro_marco, sizeof(uint32_t), 0);
		log_info(logger, "Recibido numero de marco: %d", nro_marco);

		if(!marco_en_tlb(nro_marco,numero_pagina)){
			if(strcmp(reemplazo_tlb,"LRU") == 0)
				reemplazo_tlb_LRU(numero_pagina, nro_marco);
			else if(strcmp(reemplazo_tlb,"FIFO") == 0)
				reemplazo_tlb_FIFO(numero_pagina,nro_marco);
		}
	}

	uint32_t desplazamiento = dir_logica - numero_pagina * tam_pagina;

	dire_fisica.marco = nro_marco;
	dire_fisica.desplazamiento =desplazamiento;

	return dire_fisica;
}



bool pagina_marco_tlb(TLB_t* entrada,int32_t marco,int32_t pagina){//Para encontrar la lista a actualizar
	return (entrada->marco== marco && entrada->pagina == pagina);
}

int32_t presente_en_tlb(uint32_t numero_pagina){

	for(int i=0;i< entradas_tlb;i++){
		TLB_t *tlb_aux = tlb_aux=list_get(tlb,i);
//		log_info(logger,"Numero pagina: %d",tlb_aux->pagina);
//		log_info(logger,"Numero marco: %d", tlb_aux->marco);
//		log_info(logger,"Ciclo cpu: %d",(int)tlb_aux->ultima_referencia);
		if(tlb_aux->pagina==numero_pagina){
			tlb_aux->ultima_referencia = clock();
			log_info(logger,"TLB HIT (pagina %d)", numero_pagina);
			return tlb_aux->marco;
		}


	}

	return -1;
}

void reemplazo_tlb_FIFO(uint32_t numero_pagina, int32_t marco ){
	TLB_t* tlb_entrada_0 = list_get(tlb,0);
	log_info(logger,"Pagina %d asignada al marco %d", numero_pagina, marco);
	if (tlb_entrada_0->pagina != -1)
		log_info(logger, "Pagina %d y marco %d reemplazados :(", tlb_entrada_0->pagina, tlb_entrada_0->marco);
	list_remove_and_destroy_element(tlb,0,free);
	TLB_t *tlb_entrada = crear_entrada_tlb(numero_pagina,marco);
	list_add(tlb,tlb_entrada);

}

bool menor(TLB_t* a,TLB_t* b){
	return a->ultima_referencia<b->ultima_referencia;
}


void reemplazo_tlb_LRU(uint32_t numero_pagina, int32_t marco ){

	list_sort(tlb,*menor); //Preguntar como se supone que funciona esto
	TLB_t* tlb_entrada_0 = list_get(tlb,0);
	log_info(logger,"Pagina %d asignada al marco %d", numero_pagina, marco);
	if (tlb_entrada_0->pagina != -1)
		log_info(logger, "Pagina %d y marco %d reemplazados :(", tlb_entrada_0->pagina, tlb_entrada_0->marco);
	list_remove_and_destroy_element(tlb,0,free);
	TLB_t *tlb_entrada = crear_entrada_tlb(numero_pagina,marco);
	list_add(tlb,tlb_entrada);
}

bool marco_en_tlb(int32_t marco,int32_t pagina){//Para encontrar la lista a actualizar
	for (int i=0;i< entradas_tlb;i++){
		TLB_t *tlb_aux = list_get(tlb,i);
		if(tlb_aux->marco==marco){
			list_remove(tlb,i);
			tlb_aux->pagina = pagina;
			tlb_aux->ultima_referencia = clock();
			list_add(tlb,tlb_aux);
			return true;
			}
	}
	return false;
}

int execute(instruccion_t* instruccion_ejecutar,uint32_t tabla_paginas){

	switch (instruccion_ejecutar->operacion) {
		case 'N':
			log_info(logger,"Ejecutando NOOP");
			usleep(espera * 1000);
			break;
		case 'I':
			log_info(logger,"Ejecutando IO");
			return BLOCKED;
			break;
		case 'R':
			log_info(logger,"Ejecutando READ");

			log_info(logger,"Valor leido %d",ejecutarRead(instruccion_ejecutar->arg1,tabla_paginas));
			break;
		case 'C': //Hizo read en fetch operands
			log_info(logger,"Ejecutando COPY");
			ejecutarWrite(instruccion_ejecutar->arg1,instruccion_ejecutar->arg2,tabla_paginas);
			break;
		case 'W':
			log_info(logger,"Ejecutando WRITE");
			ejecutarWrite(instruccion_ejecutar->arg1,instruccion_ejecutar->arg2,tabla_paginas);
			break;
		case 'E':
			log_info(logger,"Ejecutando EXIT");
			return EXIT;
			break;
		default:
			log_error(logger,"La instruccion se la come");
			break;
	}

	return CONTINUE;
}

void interrupcion(){
	//cambia hay_interrupcion a true
	while(1){
		op_code opcode;
		recv(cliente_socket_interrupt, &opcode, sizeof(op_code), 0);
//		log_info(logger, "Interrupcion recibida");
		pthread_mutex_lock(&mx_hay_interrupcion);
		hay_interrupcion = true;
		pthread_mutex_unlock(&mx_hay_interrupcion);
	}
}
