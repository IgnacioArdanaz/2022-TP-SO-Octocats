#include "memoria_utils.h"

pthread_mutex_t mx_log = PTHREAD_MUTEX_INITIALIZER;
int cliente_kernel;
int cliente_cpu;

// Tablas de páginas
t_list* lista_tablas_1er_nivel;
t_list* lista_tablas_2do_nivel;

// Estructura clock
t_list* lista_estructuras_clock;

// Memoria real
uint8_t* bitarray_marcos_ocupados;
void* memoria;

// Datos config
char* algoritmo;
uint16_t tam_memoria;
uint16_t tam_pagina;
uint16_t entradas_por_tabla;
uint16_t retardo_memoria;
uint16_t marcos_por_proceso;

// Extras
uint16_t pid_actual;

void imprimir_tablas_1(){
	printf("TABLAS 1ER NIVEL:\n");
	for (int i = 0; i < list_size(lista_tablas_1er_nivel); i++){
		printf("TABLA %d:\n", i);
		fila_1er_nivel* tabla = list_get(lista_tablas_1er_nivel, i);
		for (int j = 0; j < entradas_por_tabla; j ++){
			printf("%d - %d\n", j, tabla[j]);
		}
	}
}

void imprimir_tablas_2(){
	printf("TABLAS 2DO NIVEL:\n");
	for (int i = 0; i < list_size(lista_tablas_2do_nivel); i++){
		printf("TABLA %d:\n", i);
		fila_2do_nivel* tabla = list_get(lista_tablas_2do_nivel, i);
		for (int j = 0; j < entradas_por_tabla; j ++){
			printf("%d - marco %d uso %d presencia %d modificado %d\n", j, tabla[j].nro_marco,tabla[j].uso, tabla[j].presencia, tabla[j].modificado);
		}
	}
}

void inicializar_memoria(){
	logger = log_create("memoria.log", "memoria", 1, LOG_LEVEL_INFO);
	config = config_create("memoria.config");

	tam_memoria = config_get_int_value(config,"TAM_MEMORIA");
	tam_pagina = config_get_int_value(config,"TAM_PAGINA");
	entradas_por_tabla = config_get_int_value(config,"ENTRADAS_POR_TABLA");
	retardo_memoria = config_get_int_value(config,"RETARDO_MEMORIA");
	marcos_por_proceso = config_get_int_value(config,"MARCOS_POR_PROCESO");
	algoritmo = config_get_string_value(config,"ALGORITMO_REEMPLAZO");

	char* ip = config_get_string_value(config,"IP");
	char* puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA");
	server_memoria = iniciar_servidor(logger, "MEMORIA", ip, puerto_escucha);
	cliente_cpu = esperar_cliente(logger, "MEMORIA", server_memoria);

	log_info(logger, "Enviando a CPU: tam_pagina=%d - cant_ent_paginas=%d", tam_pagina, entradas_por_tabla);
	send(cliente_cpu, &entradas_por_tabla, sizeof(uint16_t),0);
	send(cliente_cpu, &tam_pagina, sizeof(uint16_t),0);
//	send_datos_necesarios(cliente_cpu, entradas_por_tabla, tam_pagina);
	cliente_kernel = esperar_cliente(logger, "MEMORIA", server_memoria);

	memoria = malloc(tam_memoria);
	lista_tablas_1er_nivel = list_create();
	lista_tablas_2do_nivel = list_create();
	lista_estructuras_clock = list_create();
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
			log_error(logger,"DISCONNECT FAILURE!");
			pthread_mutex_unlock(&mx_log);
			return;
		}
		switch (cop) {
			case CREAR_TABLA:
			{
				log_info(logger, "Entre a crear tabla");
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

				log_info(logger, "Creando tabla para programa %d de tamanio %d", pid, tamanio);

				uint32_t tabla_paginas = crear_tablas(pid, tamanio);

				log_info(logger, "Tabla creada: Número %d\n", tabla_paginas);
				send(cliente_kernel, &tabla_paginas, sizeof(uint32_t), 0);

				return;
			}
			case SUSPENDER_PROCESO:
			{
				log_info(logger, "Entre a susp proc");
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

				log_info(logger, "Suspendiendo proceso %d (tabla_paginas = %d)", pid, tabla_paginas);

				suspender_proceso(pid, tabla_paginas);

				uint16_t resultado = 1;
				send(cliente_kernel, &resultado, sizeof(uint16_t), 0);


				return;
			}
			case ELIMINAR_ESTRUCTURAS:
			{
				log_info(logger, "Entre a eliminar estructuras");
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

				log_info(logger, "Eliminando tablas del proceso %d", pid);
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
	FILE* swap = crear_archivo_swap(pid);
	inicializar_tabla_1er_nivel(tabla_1er_nivel); //Pone todas las entradas en -1
	for (int i = 0; i < entradas_por_tabla && marcos_req > marcos_actuales(i, 0); i++){
		fila_2do_nivel* tabla_2do_nivel = malloc(entradas_por_tabla * sizeof(fila_2do_nivel));
		inicializar_tabla_2do_nivel(tabla_2do_nivel); //Pone todas las entradas en -1
		for (int j = 0 ; j < entradas_por_tabla && marcos_req > marcos_actuales(i, j); j++){
			uint32_t nro_marco = agregar_marco_en_swap(swap, tam_pagina);
			fila_2do_nivel fila = {nro_marco,0,0,0}; //Podria ser {0,0,0,0}
			tabla_2do_nivel[j] = fila;
		}
		uint32_t nro_tabla_2do_nivel = list_add(lista_tablas_2do_nivel, tabla_2do_nivel);
		tabla_1er_nivel[i] = nro_tabla_2do_nivel;
	}
	cerrar_archivo_swap(swap);
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
	printf("Antes de abrir arch swap \n");
	FILE* swap = abrir_archivo_swap(pid);
	printf("Desp de abrir arch swap \n");
	estructura_clock* estructura = buscar_estructura_clock(pid);
	printf("Desp de buscar estructura clock (pid = %d, puntero = %d\n", estructura->pid, estructura->puntero);
//	printf("Desp de buscar estructura clock (pid = %d, puntero = %d, size marcos = %d \n", estructura->pid, estructura->puntero, list_size(estructura->marcos_en_memoria));
	fila_estructura_clock* fila_busqueda;
	for (int i = 0; i < list_size(estructura->marcos_en_memoria); i++){
		fila_busqueda = list_get(estructura->marcos_en_memoria, i);
		bitarray_marcos_ocupados[fila_busqueda->nro_marco_en_memoria] = 0;
		if(fila_busqueda->pagina->modificado == 1){
			printf("Antes de actualizar marco\n");
			actualizar_marco_en_swap(swap, fila_busqueda->nro_marco_en_swap, obtener_marco(fila_busqueda->nro_marco_en_memoria), tam_pagina);
			printf("Desp de actualizar marco\n");
		}
		fila_busqueda->pagina->presencia = 0;
		fila_busqueda->pagina->uso = 0;
		fila_busqueda->pagina->modificado = 0;
		estructura->puntero = 0;
		printf("Antes de eliminar de estructura clock\n");
		list_remove(estructura->marcos_en_memoria, i); //Lo hago aparte porque me da miedo usar malloc
	}
	cerrar_archivo_swap(swap);
}

// FINALIZACIÓN DE PROCESO
void eliminar_estructuras(uint32_t tabla_paginas, uint16_t pid) {
	// Actualizar bitarray de marcos libres y eliminar el .swap
	// Tambien podría eliminar la estructura de la lista y sus elementos.
	estructura_clock* estructura = buscar_estructura_clock(pid);
	fila_estructura_clock* fila_busqueda;
	for (int i = 0; i < list_size(estructura->marcos_en_memoria); i++){
		fila_busqueda = list_get(estructura->marcos_en_memoria, i);
		bitarray_marcos_ocupados[fila_busqueda->nro_marco_en_memoria] = 0;
	}
	borrar_archivo_swap(pid);
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
			log_error(logger,"DISCONNECT FAILURE!");
			pthread_mutex_unlock(&mx_log);
			return;
		}
		switch (cop) {
			case SOLICITUD_NRO_TABLA_2DO_NIVEL:
			{
				log_info(logger, "Entre a sol nro tabla 2");
				uint16_t pid = 0;
				uint32_t nro_tabla_1er_nivel = 0;
				uint32_t entrada_tabla = 0;

				if (!recv_solicitud_nro_tabla_2do_nivel(cliente_cpu, &pid, &nro_tabla_1er_nivel, &entrada_tabla)) {
					pthread_mutex_lock(&mx_log);
					log_error(logger,"Fallo recibiendo SOLICITUD NRO TABLA 2DO NIVEL");
					pthread_mutex_unlock(&mx_log);
					break;
				}

				log_info(logger, "Obteniendo el numero de tabla de 2do nivel (pid = %d | nro_tabla_1er_nivel = %d | entrada_tabla = %d)", pid, nro_tabla_1er_nivel, entrada_tabla);
				fila_1er_nivel nro_tabla_2do_nivel = obtener_nro_tabla_2do_nivel(nro_tabla_1er_nivel, entrada_tabla, pid);
				log_info(logger, "Numero de tabla de 2do nivel = %d", nro_tabla_2do_nivel);

				usleep(retardo_memoria * 1000);
				send(cliente_cpu, &nro_tabla_2do_nivel, sizeof(fila_1er_nivel), 0);

				return;
			}
			case SOLICITUD_NRO_MARCO:
			{
				log_info(logger, "Entre a sol nro marco");
				uint16_t pid = 0; // No se usa por ahora
				uint32_t nro_tabla_2do_nivel = 0;
				uint32_t entrada_tabla = 0;
				uint32_t index_tabla_1er_nivel = 0; //Lo necesito para guardar el numero de pagina

				if (!recv_solicitud_nro_marco(cliente_cpu, &pid, &nro_tabla_2do_nivel, &entrada_tabla, &index_tabla_1er_nivel)) {
					pthread_mutex_lock(&mx_log);
					log_error(logger,"Fallo recibiendo SOLICITUD NRO TABLA 2DO NIVEL");
					pthread_mutex_unlock(&mx_log);
					break;
				}

				log_info(logger, "Obteniendo numero de marco (nro_tabla_2do_nivel = %d | entrada_tabla = %d | index_tabla_1er_nivel = %d)", nro_tabla_2do_nivel, entrada_tabla, index_tabla_1er_nivel);
				uint32_t nro_marco = obtener_nro_marco_memoria(nro_tabla_2do_nivel, entrada_tabla, index_tabla_1er_nivel);
				log_info(logger, "Numero de marco obtenido = %d", nro_marco);

				usleep(retardo_memoria * 1000);
				send(cliente_cpu, &nro_marco, sizeof(uint32_t), 0);

				return;
			}
			case READ:
			{
				log_info(logger, "Entre a read");
				uint32_t nro_marco;
				uint16_t desplazamiento;

				if (!recv_read(cliente_cpu, &nro_marco, &desplazamiento)) {
					pthread_mutex_lock(&mx_log);
					log_error(logger,"Fallo recibiendo SOLICITUD NRO TABLA 2DO NIVEL");
					pthread_mutex_unlock(&mx_log);
					break;
				}

				log_info(logger, "Obteniendo dato (nro_marco = %d, desplazamiento = %d)", nro_marco, desplazamiento);
				uint32_t dato = read_en_memoria(nro_marco, desplazamiento);
				log_info(logger, "Dato leido '%d'", dato);

				usleep(retardo_memoria * 1000);
				send(cliente_cpu, &dato, sizeof(uint32_t), 0);

				return;
			}
			case WRITE:
			{
				log_info(logger, "Entre a write");
				uint32_t nro_marco;
				uint16_t desplazamiento;
				uint32_t dato;

				if (!recv_write(cliente_cpu, &nro_marco, &desplazamiento, &dato)) {
					pthread_mutex_lock(&mx_log);
					log_error(logger,"Fallo recibiendo SOLICITUD NRO TABLA 2DO NIVEL");
					pthread_mutex_unlock(&mx_log);
					break;
				}

				log_info(logger, "Escribiendo '%d' (nro_marco = %d, desplazamiento = %d)", dato, nro_marco, desplazamiento);
				write_en_memoria(nro_marco, desplazamiento, dato);

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
	return tabla[index];
}

// Dados un nro de tabla de 2do nivel y un index devuelve el nro de marco correspondiente
// Si el bit de presencia esta en 0, traerlo a memoria --> algoritmos clock / clock modificado
uint32_t obtener_nro_marco_memoria(uint32_t nro_tabla, uint32_t index, uint32_t index_tabla_1er_nivel){
	fila_2do_nivel* tabla_2do_nivel = list_get(lista_tablas_2do_nivel, nro_tabla);
	fila_2do_nivel* pagina = &tabla_2do_nivel[index];
	if (pagina->presencia == 1){
		pagina->uso = 1;
		return pagina->nro_marco;
	}
	// si no esta en memoria, traerlo (HAY PAGE FAULT)
	FILE* swap = abrir_archivo_swap(pid_actual);
	void* marco = leer_marco_en_swap(swap,pagina->nro_marco, tam_pagina);
	int32_t nro_marco;
	// si ya ocupa todos los marcos que puede ocupar, hacer un reemplazo (EJECUTAR ALGORITMO)
	if (marcos_en_memoria() == marcos_por_proceso){
		printf("Reemplazo\n");
		//Devuelve el marco donde estaba la víctima, que es donde voy a colocar la nueva página
		nro_marco = usar_algoritmo(swap);
	}
	else{
		nro_marco = buscar_marco_libre();
		if (nro_marco == -1){
			printf("ERROR!!!!! NO HAY MARCOS LIBRES EN MEMORIA!!!\n");
			exit(EXIT_FAILURE);
		}
	}
	cerrar_archivo_swap(swap);
	bitarray_marcos_ocupados[nro_marco] = 1;
	pagina->nro_marco = nro_marco;
	pagina->presencia = 1;
	pagina->uso = 1;
	escribir_marco_en_memoria(pagina->nro_marco, marco);

	uint32_t nro_marco_en_swap = index_tabla_1er_nivel * entradas_por_tabla + index;
	agregar_pagina_a_estructura_clock(nro_marco, pagina, nro_marco_en_swap);
	return nro_marco;
}

// Vemos que algoritmo usar y lo usamos
uint32_t usar_algoritmo(FILE* swap){
	if (strcmp(algoritmo, "CLOCK-M") == 0)
		return clock_modificado(swap);
	else if (strcmp(algoritmo, "CLOCK") == 0)
		return clock_simple(swap);
	else{
		exit(-1);
	}
}

uint32_t clock_simple(FILE* swap){
	// hasta que no encuentre uno no para
	estructura_clock* estructura = buscar_estructura_clock(pid_actual);
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
			reemplazo_por_clock(nro_marco_en_swap, pagina, swap);
			estructura->puntero = puntero_clock; //Guardo el puntero actualizado.
			return nro_marco;
		}
		else{
			pagina->uso = 0;
		}
	}

	return 0;
}

uint32_t clock_modificado(FILE* swap){
	estructura_clock* estructura = buscar_estructura_clock(pid_actual);
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
				reemplazo_por_clock(nro_marco_en_swap, pagina, swap);
				estructura->puntero = puntero_clock; //Guardo el puntero actualizado.
				return nro_marco;
			}
			//Condición para ir al siguiente paso
			if (puntero_clock == estructura->puntero){
				puntero_clock--; //Lo retrocedo porque abajo se incrementa de nuevo
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
				reemplazo_por_clock(nro_marco_en_swap, pagina, swap);
				estructura->puntero = puntero_clock; //Guardo el puntero actualizado.
				return nro_marco;
			}
			else{
				pagina->uso = 0;
			}

			//Condición para ir al siguiente paso
			if (puntero_clock == estructura->puntero){
				puntero_clock--;
				break;
			}
		}
		// 3er paso del algoritmo, volver a empezar
	}

	return 0;
}

void reemplazo_por_clock(uint32_t nro_marco_en_swap, fila_2do_nivel* entrada_2do_nivel, FILE* swap){
	// logica de swappeo de marco
	printf("Elegido %d!\n",nro_marco_en_swap);
	// si tiene el bit de modificado en 1, hay que actualizar el archivo swap
	if (entrada_2do_nivel->modificado == 1){
		actualizar_marco_en_swap(swap, nro_marco_en_swap, obtener_marco(entrada_2do_nivel->nro_marco), tam_pagina);
		entrada_2do_nivel->modificado = 0;
	}
	entrada_2do_nivel->presencia = 0;
	bitarray_marcos_ocupados[entrada_2do_nivel->nro_marco] = 0;
}

// Dado un nro de marco y un desplazamiento, devuelve el dato en concreto
uint32_t read_en_memoria(uint32_t nro_marco, uint16_t desplazamiento){
	uint32_t desplazamiento_final = nro_marco * tam_pagina + desplazamiento;
	uint32_t dato;
	memcpy(&dato, memoria + desplazamiento_final, sizeof(dato));
	fila_2do_nivel* pagina_actual = obtener_pagina(pid_actual, nro_marco);
	pagina_actual->uso = 1;
	return dato;
}

// Dado un nro de marco, un desplazamiento y un dato, escribe el dato en dicha posicion
void write_en_memoria(uint32_t nro_marco, uint16_t desplazamiento, uint32_t dato) {
	uint32_t desplazamiento_final = nro_marco * tam_pagina + desplazamiento;
	memcpy(memoria + desplazamiento_final, &dato, sizeof(dato));
	fila_2do_nivel* pagina_actual = obtener_pagina(pid_actual, nro_marco);
	pagina_actual->uso = 1;
	pagina_actual->modificado = 1;
}

////////////////////////// FUNCIONES GENERALES //////////////////////////

// Devuelve el contenido de un marco que está en memoria.
void* obtener_marco(uint32_t nro_marco){
	void* marco = malloc(tam_pagina);
	memcpy(marco, memoria + nro_marco * tam_pagina, tam_pagina);
	return marco;
}

// Cuenta la cantidad de marcos en memoria que tiene un proceso
uint32_t marcos_en_memoria(){
	estructura_clock* estructura = buscar_estructura_clock(pid_actual);
	return list_size(estructura->marcos_en_memoria);
}

void escribir_marco_en_memoria(uint32_t nro_marco, void* marco){
	uint32_t marco_en_memoria = nro_marco * tam_pagina;
	memcpy(memoria + marco_en_memoria, marco, tam_pagina);
}


// Buscar el primer marco libre
int buscar_marco_libre(){
	for (int i = 0; i < tam_memoria / tam_pagina; i++){
		if (bitarray_marcos_ocupados[i] == 0)
			return i;
	}
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

void printear_bitmap(){
	for (int i = 0; i < tam_memoria / tam_pagina; i++){
		printf("%d : %d\n", i, bitarray_marcos_ocupados[i]);
	}
}

////////////////////////// ESTRUCTURA CLOCK //////////////////////////

void agregar_pagina_a_estructura_clock(int32_t nro_marco, fila_2do_nivel* pagina, uint32_t nro_marco_en_swap){
	//Si ya está ese marco en la estructura, actualizo su página.
	estructura_clock* estructura = buscar_estructura_clock(pid_actual);
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
	fila_estructura_clock fila_nueva;
	fila_nueva.nro_marco_en_memoria = nro_marco;
	fila_nueva.pagina = pagina;
	fila_nueva.nro_marco_en_swap = nro_marco_en_swap;
	list_add(estructura->marcos_en_memoria, &fila_nueva);
}

estructura_clock* buscar_estructura_clock(uint16_t pid_a_buscar){
	estructura_clock* estructura;
	printf("size lista_estructuras_clock = %d\n", list_size(lista_estructuras_clock));
	for (int i = 0; i < list_size(lista_estructuras_clock); i++){
		estructura = list_get(lista_estructuras_clock, i);
		printf("lista_estructuras_clock elemento %d : pid = %d \n", i, estructura->pid);
		if (pid_a_buscar == estructura->pid)
			return estructura;
	}
	printf("Estoy por devolver 0\n");
	return 0;
}

void crear_estructura_clock(uint16_t pid){
	estructura_clock* estructura = malloc(sizeof(estructura_clock));
	estructura->pid = pid;
	estructura->marcos_en_memoria = list_create();
	estructura->puntero = 0;
	printf("creando elemento: pid = %d \n", estructura->pid);
	list_add(lista_estructuras_clock, estructura);
	log_info("Creada estructura clock del proceso: %d", pid);
}


// avanza al proximo marco del proceso
uint16_t avanzar_puntero(uint16_t puntero_clock){
	if(puntero_clock + 1 == marcos_por_proceso)
		return 0;
	else
		return puntero_clock++;
}

// Devuelve el puntero a una página (fila de segundo nivel)
fila_2do_nivel* obtener_pagina(uint16_t pid_actual, int32_t nro_marco){
	estructura_clock* estructura = buscar_estructura_clock(pid_actual);
	fila_estructura_clock* fila_busqueda;
	for (int i = 0; i < list_size(estructura->marcos_en_memoria); i++){
		fila_busqueda = list_get(estructura->marcos_en_memoria, i);
		if (nro_marco == fila_busqueda->nro_marco_en_memoria){
			return fila_busqueda->pagina;
		}
	}
	return 0;
}
