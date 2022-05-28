#include "safe_log.h"

void safe_log_info(safe_log* safe_logger, char* msg){
	pthread_mutex_lock(safe_logger->mx);
	log_info(safe_logger->logger,msg);
	pthread_mutex_unlock(safe_logger->mx);
}

void safe_log_error(safe_log* safe_logger, char* msg){
	pthread_mutex_lock(safe_logger->mx);
	log_error(safe_logger->logger,msg);
	pthread_mutex_unlock(safe_logger->mx);
}

safe_log* safe_log_create(char* filename, char* process_name,
		bool is_active_console, t_log_level level){
	pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;
	safe_log* safe_logger;
	safe_logger->logger = log_create(filename, process_name, is_active_console, level);
	safe_logger->mx = &mx;
	return safe_logger;
}

void safe_log_destroy(safe_log* safe_logger){
	log_destroy(safe_logger->logger);
	pthread_mutex_destroy(safe_logger->mx);
}

