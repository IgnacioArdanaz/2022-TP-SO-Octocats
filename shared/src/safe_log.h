#ifndef SAFE_LOG_H_
#define SAFE_LOG_H_

#include <commons/log.h>
#include <pthread.h>

typedef struct{
	t_log* logger;
	pthread_mutex_t* mx;
}safe_log;

void safe_log_info(safe_log* safe_logger, char* msg);

void safe_log_error(safe_log* safe_logger, char* msg);

safe_log* safe_log_create(char* filename, char* process_name,
		bool is_active_console, t_log_level level);

void safe_log_destroy(safe_log* safe_logger);

#endif
