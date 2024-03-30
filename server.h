#ifndef SERVER_H
#define SERVER_H

int create_server_socket(int port);
void *handle_clients(void *args);

#endif // SERVER_H
