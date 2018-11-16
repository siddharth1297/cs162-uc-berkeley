#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "wq.h"

pthread_t *p_threads; 			// Threads in thread pool
int num_request = 0;  			// No of request
wq_t *wq; 						// Work Queue

pthread_mutex_t mutex; 			// For mutual exclusion
pthread_cond_t job_queue; 		// Job queue
void (*request_handler)(int); 	// Request handler


/* push request to the work queue */
void add_to_work_queue(int fd) {
	/* Push the request to the work queue */
	pthread_mutex_lock(&mutex);
	num_request++;
	wq_push(wq, fd);
	
	// Signal
	pthread_cond_signal(&job_queue);

	pthread_mutex_unlock(&mutex);
}


/* assign a thread to a request */
void* assign_thread(void * arg) {
	while(1) {
		pthread_mutex_lock(&mutex);

		if(num_request == 0) {
			pthread_cond_wait(&job_queue, &mutex);
		}
		int fd = wq_pop(wq);
		num_request--;
		
		pthread_mutex_unlock(&mutex);
		request_handler(fd);
	}
	
	return NULL;
}

/* Initialize thread pool
 * On success returns 1, Otherwise returns 0
 */
int thpool_init(int num_threads, void (*req_handler)(int)) {
	request_handler = req_handler;

	wq = (wq_t*)malloc(sizeof(wq_t));
	wq_init(wq);

	if( (p_threads = (pthread_t*) malloc(sizeof(pthread_t) * num_threads)) == NULL ) {
		fprintf(stderr, "Error in creating thread: %s\n", strerror(errno));
		return 0;
	}

	if(pthread_mutex_init(&mutex, NULL) != 0) {
		fprintf(stderr, "Error in initializing mutex: %s\n", strerror(errno));
		return 0;
	}

	if(pthread_cond_init(&job_queue, NULL) != 0) {
		fprintf(stderr, "Error in initializing condition variable: %s\n", strerror(errno));
		return 0;
	}

	int i;
	for(i=0; i<num_threads; i++) {
		if( pthread_create(&p_threads[i], NULL, &assign_thread, NULL) != 0) {
			fprintf(stderr, "Error in pthread creation: %s\n", strerror(errno));
			return 0;
		}
	}
  	return 1;

}