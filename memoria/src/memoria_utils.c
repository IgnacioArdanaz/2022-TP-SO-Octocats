#include "memoria_utils.h"

pthread_mutex_t mx_log = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_estructuras_clock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_bitarray_marcos_ocupados = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_memoria = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_fd_swaps = PTHREAD_MUTEX_INITIALIZER;

int cliente_kernel;
int cliente_cpu;

// Tablas de páginas
t_list* lista_tablas_1er_nivel;
t_list* lista_tablas_2do_nivel;

// Estructura clock
t_dictionary* estructuras_clock;

// Memoria real
uint8_t* bitarray_marcos_ocupados;
void* memoria;

// fd de swaps
t_dictionary* fd_swaps;

// Datos config
char* algoritmo;
uint16_t tam_memoria;
uint16_t tam_pagina;
uint16_t entradas_por_tabla;
uint16_t retardo_memoria;
uint16_t marcos_por_proceso;

// Extras
uint16_t pid_actual;

// funciones para no pasar a char cada vez que accedemos

int get_fd(uint16_t pid){
	char* key = string_itoa(pid);
	pthread_mutex_lock(&mx_fd_swaps);
	int fd = (int) dictionary_get(fd_swaps, key);
	pthread_mutex_unlock(&mx_fd_swaps);
	free(key);
	return fd;
}

void set_fd(uint16_t pid, int fd){
	char* key = string_itoa(pid);
	pthread_mutex_lock(&mx_fd_swaps);
	dictionary_put(fd_swaps, key, (int*) fd);
	pthread_mutex_unlock(&mx_fd_swaps);
	free(key);
}

void del_fd(uint16_t pid){
	char* key = string_itoa(pid);
	pthread_mutex_lock(&mx_fd_swaps);
	dictionary_remove(fd_swaps, key);
	pthread_mutex_unlock(&mx_fd_swaps);
	free(key);
}

estructura_clock* get_estructura_clock(uint16_t pid){
	char* key = string_itoa(pid);
	pthread_mutex_lock(&mx_estructuras_clock);
	estructura_clock* e_clock = dictionary_get(estructuras_clock, key);
	pthread_mutex_unlock(&mx_estructuras_clock);
	free(key);
	return e_clock;
}

void set_estructura_clock(uint16_t pid, estructura_clock* e_clock){
	char* key = string_itoa(pid);
	pthread_mutex_lock(&mx_estructuras_clock);
	dictionary_put(estructuras_clock, key, e_clock);
	pthread_mutex_unlock(&mx_estructuras_clock);
	free(key);
}

void del_estructura_clock(uint16_t pid){
	char* key = string_itoa(pid);
	pthread_mutex_lock(&mx_estructuras_clock);
	dictionary_remove_and_destroy(estructuras_clock, key, free);
	pthread_mutex_unlock(&mx_estructuras_clock);
	free(key);
}

////////////////////////////////////////

void inicializar_memoria(){
	logger = log_create("memoria.log", "memoria", 1, LOG_LEVEL_INFO);
	config = config_create("../memoria.config");

	t_config* config_ips = config_create("../../ips.config");
	char* ip = config_get_string_value(config_ips,"IP_MEMORIA");

	tam_memoria = config_get_int_value(config,"TAM_MEMORIA");
	tam_pagina = config_get_int_value(config,"TAM_PAGINA");
	entradas_por_tabla = config_get_int_value(config,"ENTRADAS_POR_TABLA");
	retardo_memoria = config_get_int_value(config,"RETARDO_MEMORIA");
	marcos_por_proceso = config_get_int_value(config,"MARCOS_POR_PROCESO");
	algoritmo = config_get_string_value(config,"ALGORITMO_REEMPLAZO");
	log_info(logger,"Algoritmo %s configurado", algoritmo);
	//char* ip = config_get_string_value(config,"IP");

	char* puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA");
	server_memoria = iniciar_servidor(logger, "MEMORIA", ip, puerto_escucha);
	cliente_cpu = esperar_cliente(logger, "MEMORIA", server_memoria);

	log_info(logger, "Enviando a CPU: tam_pagina=%d - cant_ent_paginas=%d", tam_pagina, entradas_por_tabla);
	send(cliente_cpu, &entradas_por_tabla, sizeof(uint16_t),0);
	send(cliente_cpu, &tam_pagina, sizeof(uint16_t),0);
	cliente_kernel = esperar_cliente(logger, "MEMORIA", server_memoria);
	config_destroy(config_ips);
	memoria = malloc(tam_memoria);
	lista_tablas_1er_nivel = list_create();
	lista_tablas_2do_nivel = list_create();
	estructuras_clock = dictionary_create();
	fd_swaps = dictionary_create();
	bitarray_marcos_ocupados = malloc(tam_memoria / tam_pagina);
	for (int i = 0; i < tam_memoria / tam_pagina; i++)
		bitarray_marcos_ocupados[i] = 0;

	inicializar_swap();
}

/***********************************************************************/
/***************************** HILO KERNEL *****************************/
/***********************************************************************/

void escuchar_kernel() {
	while(1) {
		recibir_kernel();
	}
}

void recibir_kernel() {
	op_code cop;
	while (cliente_kernel != -1) {
		if (recv(cliente_kernel, &cop, sizeof(op_code), 0) <= 0) {
			pthread_mutex_lock(&mx_log);
			log_error(logger,"DISCONNECT FAILURE EN KERNEL!");
			pthread_mutex_unlock(&mx_log);
			exit(-1);
		}
		switch (cop) {
			case CREAR_TABLA:
			{
				uint16_t tamanio = 123;
				uint16_t pid = 123;
				recv(cliente_kernel, &tamanio, sizeof(uint16_t), 0);
				recv(cliente_kernel, &pid, sizeof(uint16_t), 0);

//				if (!recv_crear_tabla(cliente_kernel, &tamanio, &pid)) {
//					pthread_mutex_lock(&mx_log);
//					log_error(logger,"Fallo recibiendo CREAR TABLA");
//					pthread_mutex_unlock(&mx_log);
//					break;
//				}

				log_info(logger, "[KERNEL] Creando tabla para programa %d de tamanio %d", pid, tamanio);

				uint32_t tabla_paginas = crear_tablas(pid, tamanio);

				log_info(logger, "[KERNEL] Tabla creada: Número %d", tabla_paginas);
				send(cliente_kernel, &tabla_paginas, sizeof(uint32_t), 0);

				return;
			}
			case SUSPENDER_PROCESO:
			{
				uint32_t tabla_paginas = 0;
				uint16_t pid = 0;
				recv(cliente_kernel, &pid, sizeof(uint16_t),0);
				recv(cliente_kernel, &tabla_paginas, sizeof(uint16_t),0);
//				if (!recv_suspender_proceso(cliente_kernel, &pid, &tabla_paginas)) {
//					pthread_mutex_lock(&mx_log);
//					log_error(logger,"Fallo recibiendo ELIMINAR ESTRUCTURAS");
//					pthread_mutex_unlock(&mx_log);
//					break;
//				}

				log_info(logger, "[KERNEL] Suspendiendo proceso %d (tabla_paginas = %d)", pid, tabla_paginas);

				suspender_proceso(pid, tabla_paginas);

				uint16_t resultado = 1;
				send(cliente_kernel, &resultado, sizeof(uint16_t), 0);


				return;
			}
			case ELIMINAR_ESTRUCTURAS:
			{
				uint32_t tabla_paginas = 0;
				uint16_t pid = 0;

				recv(cliente_kernel, &tabla_paginas, sizeof(uint32_t),0);
				recv(cliente_kernel, &pid, sizeof(uint16_t),0);

//				if (!recv_eliminar_estructuras(cliente_kernel, &pid, &tabla_paginas)) {
//					pthread_mutex_lock(&mx_log);
//					log_error(logger,"Fallo recibiendo ELIMINAR ESTRUCTURAS");
//					pthread_mutex_unlock(&mx_log);
//					break;
//				}

				log_info(logger, "[KERNEL] Eliminando tablas del proceso %d", pid);
				eliminar_estructuras(tabla_paginas, pid);

				return;
			}
			default:
				pthread_mutex_lock(&mx_log);
				log_error(logger, "Algo anduvo mal en el server de memoria\n Cop: %d",cop);
				pthread_mutex_unlock(&mx_log);
		}
	}
}

//// INICIALIZACIÓN DE PROCESO

// creas los marcos y las tablas necesarias para el proceso
uint32_t crear_tablas(uint16_t pid, uint16_t tamanio){
	int marcos_req = calcular_cant_marcos(tamanio);
	fila_1er_nivel* tabla_1er_nivel = malloc(sizeof(int32_t) * entradas_por_tabla);
	int fd = crear_archivo_swap(pid, tamanio);
	set_fd(pid, fd);
	inicializar_tabla_1er_nivel(tabla_1er_nivel); //Pone todas las entradas en -1
	int nro_pagina = 0;
	for (int i = 0; i < entradas_por_tabla && marcos_req > marcos_actuales(i, 0); i++){
		fila_2do_nivel* tabla_2do_nivel = malloc(entradas_por_tabla * sizeof(fila_2do_nivel));
		inicializar_tabla_2do_nivel(tabla_2do_nivel); //Pone todas las entradas en -1
		for (int j = 0 ; j < entradas_por_tabla && marcos_req > marcos_actuales(i, j); j++){
			fila_2do_nivel fila = {nro_pagina,0,0,0}; //Podria ser {0,0,0,0}
			nro_pagina++; // incrementamos el nro de pagina
			tabla_2do_nivel[j] = fila;
		}
		uint32_t nro_tabla_2do_nivel = list_add(lista_tablas_2do_nivel, tabla_2do_nivel);
		tabla_1er_nivel[i] = nro_tabla_2do_nivel;
	}
	crear_estructura_clock(pid);
	return list_add(lista_tablas_1er_nivel, tabla_1er_nivel);
}

void inicializar_tabla_1er_nivel(fila_1er_nivel* tabla_1er_nivel){
	for (int i = 0; i < entradas_por_tabla; i++){
		tabla_1er_nivel[i] = -1;
	}
}

void inicializar_tabla_2do_nivel(fila_2do_nivel* tabla_2do_nivel){
	for (int i = 0; i < entradas_por_tabla; i++){
		tabla_2do_nivel[i].nro_marco = -1;
	}
}

// SUSPENSIÓN DE PROCESO
void suspender_proceso(uint16_t pid, uint32_t tabla_paginas) {
	int fd = get_fd(pid);
	estructura_clock* estructura = get_estructura_clock(pid);
	fila_estructura_clock* fila_busqueda;
	// vamos agarrando el primer elemento en todas las iteraciones
	// como estamos eliminandolos de la lista, el 2do pasaria a ser el 1er y asi sucesivamente
	while (list_size(estructura->marcos_en_memoria) > 0){
		fila_busqueda = list_get(estructura->marcos_en_memoria, 0);
		pthread_mutex_lock(&mx_bitarray_marcos_ocupados);
		bitarray_marcos_ocupados[fila_busqueda->nro_marco_en_memoria] = 0;
		pthread_mutex_unlock(&mx_bitarray_marcos_ocupados);
		if(fila_busqueda->pagina->modificado == 1){
			void* marco = obtener_marco(fila_busqueda->nro_marco_en_memoria);
			actualizar_marco_en_swap(fd, fila_busqueda->nro_marco_en_swap, marco, tam_pagina);
			// aca no implementamos el retardo para no hacer uno por cada marco swappeado
		}
		fila_busqueda->pagina->presencia = 0;
		fila_busqueda->pagina->uso = 0;
		fila_busqueda->pagina->modificado = 0;
		estructura->puntero = 0;
		list_remove_and_destroy_element(estructura->marcos_en_memoria, 0, free);
	}
	usleep(retardo_swap * 1000); // hacemos un solo retardo para mantenerlo consistente
}

// FINALIZACIÓN DE PROCESO
void eliminar_estructuras(uint32_t tabla_paginas, uint16_t pid) {
	estructura_clock* estructura = get_estructura_clock(pid);
	fila_estructura_clock* fila_busqueda;
	for (int i = 0; i < list_size(estructura->marcos_en_memoria); i++){
		fila_busqueda = list_get(estructura->marcos_en_memoria, i);
		pthread_mutex_lock(&mx_bitarray_marcos_ocupados);
		bitarray_marcos_ocupados[fila_busqueda->nro_marco_en_memoria] = 0;
		pthread_mutex_unlock(&mx_bitarray_marcos_ocupados);
	}
	list_destroy_and_destroy_elements(estructura->marcos_en_memoria, free);
	borrar_archivo_swap(pid, get_fd(pid));
	del_fd(pid);
	del_estructura_clock(pid);
}

/***********************************************************************/
/******************************* HILO CPU ******************************/
/***********************************************************************/

void escuchar_cpu() {
	while(1) {
		recibir_cpu();
	}
}

void recibir_cpu() {
	op_code cop;
	while (cliente_cpu != -1) {
		if (recv(cliente_cpu, &cop, sizeof(op_code), 0) <= 0) {
			pthread_mutex_lock(&mx_log);
			log_error(logger,"DISCONNECT FAILURE EN CPU!");
			pthread_mutex_unlock(&mx_log);
			exit(-1);
		}
		switch (cop) {
			case SOLICITUD_NRO_TABLA_2DO_NIVEL:
			{
				log_info(logger, "[CPU][ACCESO A MEMORIA] Procesando solicitud de nro tabla de 2do nivel...");
				uint16_t pid = 0;
				uint32_t nro_tabla_1er_nivel = 0;
				uint32_t entrada_tabla = 0;

				recv(cliente_cpu, &pid, sizeof(uint16_t),0);
				recv(cliente_cpu, &nro_tabla_1er_nivel, sizeof(uint32_t),0);
				recv(cliente_cpu, &entrada_tabla, sizeof(uint32_t),0);

				fila_1er_nivel nro_tabla_2do_nivel = obtener_nro_tabla_2do_nivel(nro_tabla_1er_nivel, entrada_tabla, pid);

				usleep(retardo_memoria * 1000);
				send(cliente_cpu, &nro_tabla_2do_nivel, sizeof(fila_1er_nivel), 0);

				return;
			}
			case SOLICITUD_NRO_MARCO:
			{
				log_info(logger, "[CPU][ACCESO A MEMORIA] Procesando solicitud de nro marco...");
				uint32_t nro_tabla_2do_nivel = 0;
				uint32_t entrada_tabla = 0;
				uint32_t index_tabla_1er_nivel = 0; //Lo necesito para guardar el numero de pagina

				recv(cliente_cpu, &pid_actual, sizeof(uint16_t),0);
				recv(cliente_cpu, &nro_tabla_2do_nivel, sizeof(int32_t),0);
				recv(cliente_cpu, &entrada_tabla, sizeof(uint32_t),0);
				recv(cliente_cpu, &index_tabla_1er_nivel, sizeof(uint32_t),0);

				uint32_t nro_marco = obtener_nro_marco_memoria(nro_tabla_2do_nivel, entrada_tabla, index_tabla_1er_nivel);
				log_info(logger, "[CPU] Numero de marco obtenido = %d", nro_marco);

				usleep(retardo_memoria * 1000);
				send(cliente_cpu, &nro_marco, sizeof(uint32_t), 0);

				return;
			}
			case READ:
			{
				log_info(logger, "[CPU][ACCESO A MEMORIA] Procesando lectura...");
				uint32_t nro_marco;
				uint32_t desplazamiento;

				recv(cliente_cpu, &nro_marco, sizeof(uint32_t),0);
				recv(cliente_cpu, &desplazamiento, sizeof(uint32_t),0);

				uint32_t dato = read_en_memoria(nro_marco, desplazamiento);

				usleep(retardo_memoria * 1000);
				send(cliente_cpu, &dato, sizeof(uint32_t), 0);

				return;
			}
			case WRITE:
			{
				log_info(logger, "[CPU][ACCESO A MEMORIA] Procesando escritura...");
				uint32_t nro_marco;
				uint32_t desplazamiento;
				uint32_t dato;

				recv(cliente_cpu, &nro_marco, sizeof(uint32_t),0);
				recv(cliente_cpu, &desplazamiento, sizeof(uint32_t),0);
				recv(cliente_cpu, &dato, sizeof(uint32_t),0);

				write_en_memoria(nro_marco, desplazamiento, dato);
				log_info(logger, "[CPU] Dato escrito: '%d' (nro_marco = %d, desplazamiento = %d)", dato, nro_marco, desplazamiento);

				op_code resultado = ESCRITURA_OK;
				usleep(retardo_memoria * 1000);
				send(cliente_cpu, &resultado, sizeof(op_code), 0);

				return;
			}
			default:
				pthread_mutex_lock(&mx_log);
				log_error(logger, "Algo anduvo mal en el server de memoria (EN EL HILO DE CPU)\n Cop: %d",cop);
				pthread_mutex_unlock(&mx_log);
		}
	}
}

// Dados un nro de tabla y un index devuelve el nro de tabla de 2do nivel
fila_1er_nivel obtener_nro_tabla_2do_nivel(int32_t nro_tabla, uint32_t index, uint32_t pid){
	if (pid != pid_actual){
		pid_actual = pid;
	}
	fila_1er_nivel* tabla = list_get(lista_tablas_1er_nivel,nro_tabla);
	log_info(logger, "[CPU] Tabla : %d - Index: %d - PID: %d - Resultado: %d", nro_tabla, index, pid, tabla[index]);
	if (tabla[index] == -1){
		log_error(logger, "SEGMENTATION FAULT!! El proceso no es lo suficientemente grande!");
		exit(-1);
	}
	return tabla[index];
}

// Dados un nro de tabla de 2do nivel y un index devuelve el nro de marco correspondiente
// Si el bit de presencia esta en 0, traerlo a memoria --> algoritmos clock / clock modificado
uint32_t obtener_nro_marco_memoria(uint32_t nro_tabla, uint32_t index, uint32_t index_tabla_1er_nivel){
	fila_2do_nivel* tabla_2do_nivel = list_get(lista_tablas_2do_nivel, nro_tabla);
	fila_2do_nivel* pagina = &tabla_2do_nivel[index];
	if (pagina->presencia == 1){
		log_info(logger, "[CPU] Pagina en memoria principal!");
		pagina->uso = 1;
		return pagina->nro_marco;
	}
	log_info(logger, "[CPU][ACCESO A DISCO] PAGE FAULT!!!");
	int fd = get_fd(pid_actual);
	uint32_t nro_marco_en_swap = index_tabla_1er_nivel * entradas_por_tabla + index;
	// si no esta en memoria, traerlo (HAY PAGE FAULT)
	void* marco = leer_marco_en_swap(fd,nro_marco_en_swap, tam_pagina);
	int32_t nro_marco;
	// si ya ocupa todos los marcos que puede ocupar, hacer un reemplazo (EJECUTAR ALGORITMO)
	if (marcos_en_memoria() == marcos_por_proceso){
		//Devuelve el marco donde estaba la víctima, que es donde voy a colocar la nueva página
		nro_marco = usar_algoritmo(pid_actual);
	}
	else{
		nro_marco = buscar_marco_libre();
		log_info(logger, "[CPU] El proceso tiene marcos disponibles :)");
		if (nro_marco == -1){
			log_error(logger, "ERROR!!!!! NO HAY MARCOS LIBRES EN MEMORIA!!!");
			exit(EXIT_FAILURE);
		}
	}
	pthread_mutex_lock(&mx_bitarray_marcos_ocupados);
	bitarray_marcos_ocupados[nro_marco] = 1;
	pthread_mutex_unlock(&mx_bitarray_marcos_ocupados);
	pagina->nro_marco = nro_marco;
	pagina->presencia = 1;
	pagina->uso = 1;
	escribir_marco_en_memoria(pagina->nro_marco, marco);

	free(marco);
	agregar_pagina_a_estructura_clock(nro_marco, pagina, nro_marco_en_swap);
	return nro_marco;
}

// Vemos que algoritmo usar y lo usamos
uint32_t usar_algoritmo(int pid){
	if (strcmp(algoritmo, "CLOCK-M") == 0){
		log_info(logger, "[CPU] Reemplazo por CLOCK-M");
		return clock_modificado(pid);
	}
	else if (strcmp(algoritmo, "CLOCK") == 0){
		log_info(logger, "[CPU] Reemplazo por CLOCK");
		return clock_simple(pid);
	}
	else{
		exit(-1);
	}
}

uint32_t clock_simple(int pid){
	// hasta que no encuentre uno no para
	estructura_clock* estructura = get_estructura_clock(pid_actual);
	uint16_t puntero_clock = estructura->puntero;
	fila_estructura_clock* fila;
	fila_2do_nivel* pagina;
	uint32_t nro_marco_en_swap;
	int32_t nro_marco;
	while(1){
		fila = list_get(estructura->marcos_en_memoria, puntero_clock);
		pagina = fila->pagina;
		nro_marco_en_swap = fila->nro_marco_en_swap;
		nro_marco = fila->nro_marco_en_memoria;
		puntero_clock = avanzar_puntero(puntero_clock);

		if (pagina->uso == 0){ //Encontró a la víctima
			reemplazo_por_clock(nro_marco_en_swap, pagina, pid);
			log_info(logger, "[CPU] Posición final puntero: %d",puntero_clock);
			estructura->puntero = puntero_clock; //Guardo el puntero actualizado.
			return nro_marco;
		}
		else{
			pagina->uso = 0;
		}
	}

	return 0;
}

uint32_t clock_modificado(int pid){
	estructura_clock* estructura = get_estructura_clock(pid_actual);
	uint16_t puntero_clock = estructura->puntero;
	fila_estructura_clock* fila;
	fila_2do_nivel* pagina;
	uint32_t nro_marco_en_swap;
	int32_t nro_marco;
	// hasta que no encuentre uno no para
	while(1){
		// 1er paso del algoritmo: Buscar (0,0)
		while(1){
			fila = list_get(estructura->marcos_en_memoria, puntero_clock);
			pagina = fila->pagina;
			nro_marco_en_swap = fila->nro_marco_en_swap;
			nro_marco = fila->nro_marco_en_memoria;
			puntero_clock = avanzar_puntero(puntero_clock);

			if (pagina->uso == 0 && pagina->modificado == 0){
				reemplazo_por_clock(nro_marco_en_swap, pagina, pid);
				log_info(logger, "[CPU] Posición final puntero: %d",puntero_clock);
				estructura->puntero = puntero_clock; //Guardo el puntero actualizado.
				return nro_marco;
			}
			//Condición para ir al siguiente paso
			if (puntero_clock == estructura->puntero){
				break;
			}
		}
		// 2do paso del algoritmo: Buscar (0,1) y pasar Bit de uso a 0
		while(1){
			fila = list_get(estructura->marcos_en_memoria, puntero_clock);
			pagina = fila->pagina;
			nro_marco_en_swap = fila->nro_marco_en_swap;
			nro_marco = fila->nro_marco_en_memoria;
			puntero_clock = avanzar_puntero(puntero_clock);

			if (pagina->uso == 0 && pagina->modificado == 1){
				reemplazo_por_clock(nro_marco_en_swap, pagina, pid);
				estructura->puntero = puntero_clock; //Guardo el puntero actualizado.
				return nro_marco;
			}
			else{
				pagina->uso = 0;
			}

			//Condición para ir al siguiente paso
			if (puntero_clock == estructura->puntero){
				break;
			}
		}
		// 3er paso del algoritmo, volver a empezar
	}

	return 0;
}

void reemplazo_por_clock(uint32_t nro_marco_en_swap, fila_2do_nivel* entrada_2do_nivel, int pid){
	// logica de swappeo de marco
	log_info(logger, "[CPU] Pagina victima elegida: %d",nro_marco_en_swap);
	// si tiene el bit de modificado en 1, hay que actualizar el archivo swap
	if (entrada_2do_nivel->modificado == 1){
		log_info(logger,"[CPU][ACCESO A DISCO] Actualizando pagina victima en swap (bit de modificado = 1)");
		actualizar_marco_en_swap(get_fd(pid), nro_marco_en_swap, obtener_marco(entrada_2do_nivel->nro_marco), tam_pagina);
		usleep(retardo_swap * 1000); // tenemos el retardo por swappear un marco modificado
		entrada_2do_nivel->modificado = 0;
	}
	entrada_2do_nivel->presencia = 0;
	pthread_mutex_lock(&mx_bitarray_marcos_ocupados);
	bitarray_marcos_ocupados[entrada_2do_nivel->nro_marco] = 0;
	pthread_mutex_unlock(&mx_bitarray_marcos_ocupados);
}

// Dado un nro de marco y un desplazamiento, devuelve el dato en concreto
uint32_t read_en_memoria(uint32_t nro_marco, uint32_t desplazamiento){
	uint32_t desplazamiento_final = nro_marco * tam_pagina + desplazamiento;
	uint32_t dato;
	pthread_mutex_lock(&mx_memoria);
	memcpy(&dato, memoria + desplazamiento_final, sizeof(dato));
	pthread_mutex_unlock(&mx_memoria);
	fila_2do_nivel* pagina_actual = obtener_pagina(pid_actual, nro_marco);
	pagina_actual->uso = 1;
	log_info(logger, "[CPU] Dato '%d' leido en marco %d", dato, nro_marco);
	return dato;
}

// Dado un nro de marco, un desplazamiento y un dato, escribe el dato en dicha posicion
void write_en_memoria(uint32_t nro_marco, uint32_t desplazamiento, uint32_t dato) {
	uint32_t desplazamiento_final = nro_marco * tam_pagina + desplazamiento;
	pthread_mutex_lock(&mx_memoria);
	memcpy(memoria + desplazamiento_final, &dato, sizeof(dato));
	pthread_mutex_unlock(&mx_memoria);
	fila_2do_nivel* pagina_actual = obtener_pagina(pid_actual, nro_marco);
	pagina_actual->uso = 1;
	pagina_actual->modificado = 1;
}

////////////////////////// FUNCIONES GENERALES //////////////////////////

// Devuelve el contenido de un marco que está en memoria.
void* obtener_marco(uint32_t nro_marco){
	void* marco = malloc(tam_pagina);
	pthread_mutex_lock(&mx_memoria);
	memcpy(marco, memoria + nro_marco * tam_pagina, tam_pagina);
	pthread_mutex_unlock(&mx_memoria);
	return marco;
}

// Cuenta la cantidad de marcos en memoria que tiene un proceso
uint32_t marcos_en_memoria(){
	estructura_clock* estructura = get_estructura_clock(pid_actual);
	return list_size(estructura->marcos_en_memoria);
}

void escribir_marco_en_memoria(uint32_t nro_marco, void* marco){
	uint32_t marco_en_memoria = nro_marco * tam_pagina;
	pthread_mutex_lock(&mx_memoria);
	memcpy(memoria + marco_en_memoria, marco, tam_pagina);
	pthread_mutex_unlock(&mx_memoria);
}


// Buscar el primer marco libre
int buscar_marco_libre(){
	pthread_mutex_lock(&mx_bitarray_marcos_ocupados);
	for (int i = 0; i < tam_memoria / tam_pagina; i++){
		if (bitarray_marcos_ocupados[i] == 0){
			pthread_mutex_unlock(&mx_bitarray_marcos_ocupados);
			return i;
		}
	}
	pthread_mutex_unlock(&mx_bitarray_marcos_ocupados);
	return -1;
}

// Devuelve la cantidad de marcos que requiere un proceso del tamanio especificado
uint32_t calcular_cant_marcos(uint16_t tamanio){
	int  cant_marcos = tamanio / tam_pagina;
	if (tamanio % tam_pagina != 0)
		cant_marcos++;
	return cant_marcos;
}

// Devuelve el nro de marcos que se crearon hasta el momento
int marcos_actuales(int entrada_1er_nivel, int entrada_2do_nivel){
	return entrada_1er_nivel * entradas_por_tabla + entrada_2do_nivel;
}

////////////////////////// ESTRUCTURA CLOCK //////////////////////////

void agregar_pagina_a_estructura_clock(int32_t nro_marco, fila_2do_nivel* pagina, uint32_t nro_marco_en_swap){
	//Si ya está ese marco en la estructura, actualizo su página.
	estructura_clock* estructura = get_estructura_clock(pid_actual);
	fila_estructura_clock* fila_busqueda;
	for (int i = 0; i < list_size(estructura->marcos_en_memoria); i++){
		fila_busqueda = list_get(estructura->marcos_en_memoria, i);
		if (nro_marco == fila_busqueda->nro_marco_en_memoria){
			fila_busqueda->pagina = pagina;
			fila_busqueda->nro_marco_en_swap = nro_marco_en_swap;
			return;
		}
	}
	//Si no está el marco en la estructura, lo agrego al final
	fila_estructura_clock* fila_nueva = malloc(sizeof(fila_estructura_clock));
	fila_nueva->nro_marco_en_memoria = nro_marco;
	fila_nueva->pagina = pagina;
	fila_nueva->nro_marco_en_swap = nro_marco_en_swap;
	list_add(estructura->marcos_en_memoria, fila_nueva);

}

void crear_estructura_clock(uint16_t pid){
	estructura_clock* estructura = malloc(sizeof(estructura_clock));
	estructura->pid = pid;
	estructura->marcos_en_memoria = list_create();
	estructura->puntero = 0;
	set_estructura_clock(pid, estructura);
	log_info(logger,"[CPU] Creada estructura clock del proceso: %d", pid);
}


// avanza al proximo marco del proceso
uint16_t avanzar_puntero(uint16_t puntero_clock){
    puntero_clock++;
    if(puntero_clock == marcos_por_proceso)
        return 0;
    return puntero_clock;
}

// Devuelve el puntero a una página (fila de segundo nivel)
fila_2do_nivel* obtener_pagina(uint16_t pid_actual, int32_t nro_marco){
	estructura_clock* estructura = get_estructura_clock(pid_actual);
	fila_estructura_clock* fila_busqueda;
	for (int i = 0; i < list_size(estructura->marcos_en_memoria); i++){
		fila_busqueda = list_get(estructura->marcos_en_memoria, i);
		if (nro_marco == fila_busqueda->nro_marco_en_memoria){
			return fila_busqueda->pagina;
		}
	}
	log_error(logger,"Pagina no encontrada :(");
	return NULL;
}
