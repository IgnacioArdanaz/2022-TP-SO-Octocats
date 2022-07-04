#include "cpu_main.h"

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
uint16_t pid_anterior=0;

int main(void) {

	inicializar_cpu();

	while(1){
		PCB_t* pcb = pcb_create();
		op_code cop;
		recv(cliente_socket_dispatch, &cop, sizeof(op_code), 0);
		recv_proceso(cliente_socket_dispatch,pcb);
		log_info(logger,"Proceso %d -> program counter %d - est %f", pcb->pid, pcb->pc, pcb->est_rafaga);
		if(pcb->pid != pid_anterior ) limpiar_tlb();
		struct timespec start;
		clock_gettime(CLOCK_REALTIME,&start);
		op_code estado = iniciar_ciclo_instruccion(pcb);
		struct timespec end;
		clock_gettime(CLOCK_REALTIME, &end);
		rafaga_real = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
		calculo_estimacion(pcb, estado);
		pid_anterior = pcb->pid;
		log_info(logger,"Program counter %d (despues de ejecutar)",pcb->pc);
		log_info(logger,"==============================================================");
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
	config = config_create("cpu.config");
	espera = config_get_int_value(config, "RETARDO_NOOP");
	entradas_tlb = config_get_int_value(config, "ENTRADAS_TLB");
	reemplazo_tlb = config_get_string_value(config, "REEMPLAZO_TLB");
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

	//conexion con MEMORIA
	char* ip_memoria = config_get_string_value(config,"IP_MEMORIA");
	char* puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");
	int conexion_memoria = crear_conexion(logger, "MEMORIA", ip_memoria, puerto_memoria);

	if (conexion_memoria == 0){
		log_error(logger,"Error al intentar conectarse a memoria :-(");
		exit(EXIT_FAILURE);
		recv_datos_necesarios(conexion_memoria,&tam_pagina, &cant_ent_paginas);
	}

	char* ip = config_get_string_value(config, "IP");
	char* puerto_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	char* puerto_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
	server_cpu_dispatch = iniciar_servidor(logger, "CPU_DISPATCH", ip, puerto_dispatch);
	server_cpu_interrupt = iniciar_servidor(logger, "CPU_INTERRUPT", ip, puerto_interrupt);

	cliente_socket_dispatch = esperar_cliente(logger, "CPU_DISPATCH",server_cpu_dispatch);
	cliente_socket_interrupt = esperar_cliente(logger, "CPU_INTERRUPT", server_cpu_interrupt);
	recv(cliente_socket_dispatch, &alfa, sizeof(double), 0);
	log_info(logger,"Valor alfa %.2f",alfa);
	pthread_t hilo_interrupciones;
	pthread_create(&hilo_interrupciones,NULL,(void*)interrupcion,NULL);
	pthread_detach(hilo_interrupciones);

}


void inicializar_tlb(){

	for(int i=0;i< entradas_tlb;i++){
		TLB_t *tlb_aux = crear_entrada_tlb(-1,-1);
		list_add(tlb, tlb_aux);

		//TLB_t *tlb_aux2 = list_get(tlb,i);
		//log_info(logger,"Numero pagina: %d", tlb_aux2->pagina);
		//log_info(logger,"Numero pagina: %d", tlb_aux2->marco);
	}

}


void limpiar_tlb(){
	list_clean(tlb);
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
	if (hay_interrupcion){
		hay_interrupcion = false;
		return INTERRUPTION;
	}
	return CONTINUE;
}

int fetch_operands(int direccion, uint32_t tabla){
	return ejecutarRead(direccion,tabla);
}


int decode(instruccion_t* instruccion_ejecutar ){
	return instruccion_ejecutar->operacion == 'C';
}



int ejecutarRead(uint32_t dir_logica,uint32_t tabla_paginas){
	uint32_t dir_fisica;
	int valor=8;
	dir_fisica = traducir_direccion(dir_logica,tabla_paginas);
	//Sería send_read(int fd, uint32_t nro_marco, uint16_t desplazamiento)
	//send_instruccion(dir_fisica);
	// Este de aca abajo te lo hacemos nosotros, tranqui
	// valor = recv_valor_leido();
	return valor;
}



void ejecutarWrite(uint32_t dir_logica,uint32_t valor,uint32_t tabla_paginas ){
	uint32_t dir_fisica;
	dir_fisica = traducir_direccion(dir_logica, tabla_paginas);
	//Sería send_write(int fd, uint32_t nro_marco, uint16_t desplazamiento, uint32_t dato)
	//send_instruccion(dir_fisica,valor)
	// Este de aca abajo te lo hacemos nosotros, tranqui
	//recv_verificacion
}

uint32_t traducir_direccion(uint32_t dir_logica,uint32_t tabla_paginas){

	int32_t marco;
	uint32_t numero_pagina = floor((double)dir_logica/(double)tam_pagina);
	//Aca hay que fijarse si esta en la tlb
	marco=presente_en_tlb(numero_pagina);

	if(marco==-1){
		uint32_t entrada_tabla_1 = floor((double)numero_pagina/(double)cant_ent_paginas);
		uint32_t entrada_tabla_2 = numero_pagina % cant_ent_paginas;
		// Seria send_solicitud_nro_tabla_2do_nivel(int fd, uint16_t pid, uint32_t nro_tabla_1er_nivel, uint32_t entrada_tabla)
		//send_tabla(tabla_paginas,entrada_tabla_1);
		// Este de aca abajo te lo hacemos nosotros, tranqui
		//recv_tabla(&tabla_paginas2);
		// Seria send_solicitud_nro_marco(int fd, uint16_t pid, uint32_t nro_tabla_2do_nivel, uint32_t entrada_tabla, uint32_t index_tabla_1er_nivel)
		//send_tabla(tabla_paginas2,entrada_tabla_2);
		// Este de aca abajo te lo hacemos nosotros, tranqui
		//recv_tabla(&marco);
		marco = rand()%11;
		if(!marco_en_tlb(marco,numero_pagina)){
			if(strcmp(reemplazo_tlb,"LRU"))reemplazo_tlb_LRU( numero_pagina,  marco );
			else if(strcmp(reemplazo_tlb,"FIFO")) reemplazo_tlb_FIFO(numero_pagina,marco);
		}
	}

	uint32_t desplazamiento = dir_logica - numero_pagina * tam_pagina;

	return marco + desplazamiento;


}



bool pagina_marco_tlb(TLB_t* entrada,int32_t marco,int32_t pagina){//Para encontrar la lista a actualizar
	return (entrada->marco== marco && entrada->pagina == pagina);
}

int32_t presente_en_tlb(uint32_t numero_pagina){

	for(int i=0;i< entradas_tlb;i++){
		TLB_t *tlb_aux = tlb_aux=list_get(tlb,i);
		log_info(logger,"Numero pagina: %d",tlb_aux->pagina);
		log_info(logger,"Numero marco: %d", tlb_aux->marco);
		log_info(logger,"Ciclo cpu: %d",(int)tlb_aux->ultima_referencia);
		if(tlb_aux->pagina==numero_pagina){
			tlb_aux->ultima_referencia = clock();
			return tlb_aux->marco;
		}


	}

	return -1;
}

void reemplazo_tlb_FIFO(uint32_t numero_pagina, int32_t marco ){
	list_remove_and_destroy_element(tlb,0,free);
	TLB_t *tlb_entrada = crear_entrada_tlb(numero_pagina,marco);
	list_add_in_index(tlb,entradas_tlb-1,tlb_entrada);

}

bool menor(TLB_t* a,TLB_t* b){
	return a->ultima_referencia<b->ultima_referencia;
}


void reemplazo_tlb_LRU(uint32_t numero_pagina, int32_t marco ){

	list_sort(tlb,*menor); //Preguntar como se supone que funciona esto
	list_remove_and_destroy_element(tlb,0,free);
	TLB_t *tlb_entrada = crear_entrada_tlb(numero_pagina,marco);
	list_add_in_index(tlb,entradas_tlb-1,tlb_entrada);
}

bool marco_en_tlb(int32_t marco,int32_t pagina){//Para encontrar la lista a actualizar
	for (int i=0;i< entradas_tlb;i++){
		TLB_t *tlb_aux = list_get(tlb,i);
		if(tlb_aux->marco==marco){
			tlb_aux->pagina = pagina;
			tlb_aux->ultima_referencia = clock();
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
		log_info(logger, "Interrupcion recibida");
		hay_interrupcion = true;
	}
}
