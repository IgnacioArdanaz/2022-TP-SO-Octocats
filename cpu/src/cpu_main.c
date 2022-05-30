#include "cpu_main.h"

t_log* logger;
t_config* config;
int espera, server_cpu_dispatch, server_cpu_interrupt, cliente_socket_interrupt,
	cliente_socket_dispatch;
bool hay_interrupcion;

int main(void) {

	inicializar_cpu();

	while(1){
		PCB_t* pcb = pcb_create();
		op_code cop;
		recv(cliente_socket_dispatch, &cop, sizeof(op_code), 0);
		recv_proceso(cliente_socket_dispatch,pcb);
		log_info(logger,"Program counter %d",pcb->pc);

		op_code estado = iniciar_ciclo_instruccion(pcb);

		log_info(logger,"Program counter %d (despues de ejecutar)",pcb->pc);
		send_proceso(cliente_socket_dispatch,pcb,estado);
		pcb_destroy(pcb);
	}

	apagar_cpu();

	return EXIT_SUCCESS;
}

void inicializar_cpu(){
	logger = log_create("cpu.log", "cpu", 1, LOG_LEVEL_INFO);
	config = config_create("cpu.config");
	espera = config_get_int_value(config, "RETARDO_NOOP");

	hay_interrupcion = false;

	char* puerto_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	char* puerto_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
	server_cpu_dispatch = iniciar_servidor(logger, "CPU_DISPATCH", IP, puerto_dispatch);
	server_cpu_interrupt = iniciar_servidor(logger, "CPU_INTERRUPT", IP, puerto_interrupt);

	cliente_socket_dispatch = esperar_cliente(logger, "CPU_DISPATCH",server_cpu_dispatch);
	cliente_socket_interrupt = esperar_cliente(logger, "CPU_INTERRUPT", server_cpu_interrupt);

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
			log_info(logger,"No copio nada :P");
		//Aca iria fetch operands y la ejecucion de copy
		} else {
			estado = execute(instruccion_ejecutar);
		}
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
			printf("Vas a tener que esperar perro\n");
			usleep(espera);
			break;
		case 'I':
			printf("Andate con la IO\n");
			return BLOCKED;
			break;
		case 'R':
			printf("Que queres que lea boludo\n");
			ejecutarRead();
			break;
		case 'C':
			printf("No te copio nada\n");
			ejecutarCopy();
			break;
		case 'W':
			printf("Como voy a leer sin ojos\n");
			ejecutarWrite();
			break;
		case 'E':
			printf("Hasta la proxima\n");
			return EXIT;
			break;
		default:
			log_error(logger,"La instruccion se la come\n");
			break;
	}

	return 0;
}

void interrupcion(){
	//cambia hay_interrupcion a true
	while(1){
		void* opcode;
		recv(cliente_socket_interrupt, opcode, 0, 0);
		hay_interrupcion = true;
	}
}
