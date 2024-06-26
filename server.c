#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "logger.h"
#include "fake_sensors.h"
#include "request_handler.h"

#define BACKLOG			10
#define BUFFER_LENGTH	8000


int create_server_socket(int port)
{
	// server socket
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	assert(sock != -1);
	// bind to port
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	assert(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) != -1);
	write_log(INFO, "Server socket bound with port %d.", port);
	// listen to incoming requests
	assert(listen(sock, BACKLOG) != -1);
	return sock;
}

void *handle_clients(void *args)
{
	// args
	int server_sock = (int)((long *)args)[0];
	time_t start = (time_t)((long *)args)[1];
	// buffers for storing request and response
	char request_buffer[BUFFER_LENGTH];
	char response_buffer[BUFFER_LENGTH];
	// handle clients forever and ever
	while (1) {
		write_log(DEBUG, "New session.");
		struct sockaddr_in client_sockaddr;
		socklen_t client_sockaddr_len = sizeof(client_sockaddr);
		// accept request
		int client_sock = accept(server_sock, (struct sockaddr *)&client_sockaddr, &client_sockaddr_len);
		assert(client_sock != -1);
		write_log(DEBUG, "Connected to a client.");
		// read request
		assert(read(client_sock, request_buffer, BUFFER_LENGTH) != -1);
		write_log(DEBUG, "Received request: %s", request_buffer);
		// send response
		handle_request(request_buffer, response_buffer, start);
		write(client_sock, response_buffer, strlen(response_buffer));
		write_log(DEBUG, "Sent response: %s", response_buffer);
		// close connection
		close(client_sock);
	}
	return NULL;
}
