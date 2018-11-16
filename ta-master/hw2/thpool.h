#include <pthread.h>

/* push request to the work queue */
void add_to_work_queue(int fd);

/* Initialize thread pool
 * On success returns 1, Otherwise returns 0
 */
int thpool_init(int num_threads, void (*req_handler)(int));