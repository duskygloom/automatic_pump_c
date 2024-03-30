#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <time.h>


void handle_request(const char *request_buffer, char *response_buffer, time_t start);


#endif // REQUEST_HANDLER_H
