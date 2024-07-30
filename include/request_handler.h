#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <openssl/ssl.h>

void handle_client_request(int client_socket, const char *client_ip, SSL *ssl);

#endif
