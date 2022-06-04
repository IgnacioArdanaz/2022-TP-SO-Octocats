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
int main(void) {

	inicializar_cpu();

	while(1){
		PCB_t* pcb = pcb_create();
		op_code cop;
		recv(cliente_socket_dispatch, &cop, sizeof(op_code), 0);
		recv_proceso(cliente_socket_dispatch,pcb);
		log_info(logger,"Proceso %d -> program counter %d - est %f", pcb->pid, pcb->pc, pcb->est_rafaga);

		struct timespec start;
		clock_gettime(CLOCK_REALTIME,&start);
		op_code estado = iniciar_ciclo_instruccion(pcb);
		struct timespec end;
		clock_gettime(CLOCK_REALTIME, &end);
		rafaga_real = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
		calculo_estimacion(pcb, estado);

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

	operando_copy = 0;

	//conexion con MEMORIA
//	char* ip_memoria = config_get_string_value(config,"IP_MEMORIA");
//	char* puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");
//	int conexion_memoria = crear_conexion(logger, "MEMORIA", ip_memoria, puerto_memoria);
//
//	if (conexion_memoria == 0){
//		log_error(logger,"Error al intentar conectarse a memoria :-(");
//		exit(EXIT_FAILURE);
//	}

	char* puerto_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	char* puerto_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
	server_cpu_dispatch = iniciar_servidor(logger, "CPU_DISPATCH", IP, puerto_dispatch);
	server_cpu_interrupt = iniciar_servidor(logger, "CPU_INTERRUPT", IP, puerto_interrupt);

	cliente_socket_dispatch = esperar_cliente(logger, "CPU_DISPATCH",server_cpu_dispatch);
	cliente_socket_interrupt = esperar_cliente(logger, "CPU_INTERRUPT", server_cpu_interrupt);
	recv(cliente_socket_dispatch,&alfa, sizeof(double), 0);
	log_info(logger,"Valor alfa %.2f",alfa);
	pthread_t hilo_interrupciones;
	pthread_create(&hilo_interrupciones,NULL,(void*)interrupcion,NULL);
	pthread_detach(hilo_interrupciones);

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
		//Aca iria fetch operands y la ejecucion de copy
		}
		estado = execute(instruccion_ejecutar);
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

int decode(instruccion_t* instruccion_ejecutar ){
	return instruccion_ejecutar->operacion == 'C';
}

void ejecutarIO(){

}

void ejecutarRead(){

}

void ejecutarCopy(){

}

void ejecutarWrite(){

}

int execute(instruccion_t* instruccion_ejecutar){

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
			ejecutarRead();
			break;
		case 'C':
			log_info(logger,"Ejecutando COPY");
			ejecutarCopy();
			break;
		case 'W':
			log_info(logger,"Ejecutando WRITE");
			ejecutarWrite();
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
		log_info(logger, "Interrucion recibida");
		hay_interrupcion = true;
	}
}
