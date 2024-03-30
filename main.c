#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "server.h"
#include "fake_sensors.h"

#define DEFAULT_PORT	8000


int main(int argc, char **argv)
{
	time_t start = time(0);
	// create the server socket
	int port = DEFAULT_PORT;
	if (argc > 1) port = atoi(argv[1]);
	int server_sock = create_server_socket(port);
	// create threads
	pthread_t pump_thread, server_thread;
	pthread_create(&pump_thread, NULL, &handle_sensors, NULL);
	long server_thread_args[] = { server_sock, (long)start };
	pthread_create(&server_thread, NULL, &handle_clients, server_thread_args);
	pthread_join(pump_thread, NULL);
	pthread_join(server_thread, NULL);
	
	close(server_sock);
	return 0;
}
