#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>
#include "server.h"
#include "request_handler.h"
#include "utils.h"
#include "ssl_utils.h"

#define MAX_CLIENTS 30

void start_server(int port, int use_https) {
    int server_fd, new_socket, max_sd, activity, i, sd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int client_socket[MAX_CLIENTS];
    fd_set readfds;
    SSL_CTX *ctx = NULL;

    for (i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if (use_https) {
        initialize_openssl();
        ctx = create_context();
        configure_context(ctx);
    }

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            perror("select error");
        }

        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (new_socket < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    break;
                }
            }
        }

        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];
            if (FD_ISSET(sd, &readfds)) {
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN);

                if (use_https) {
                    SSL *ssl = SSL_new(ctx);
                    SSL_set_fd(ssl, sd);

                    if (SSL_accept(ssl) <= 0) {
                        ERR_print_errors_fp(stderr);
                    } else {
                        handle_client_request(sd, client_ip, ssl);
                    }

                    SSL_free(ssl);
                } else {
                    handle_client_request(sd, client_ip, NULL);
                }

                client_socket[i] = 0;
            }
        }
    }

    if (use_https) {
        SSL_CTX_free(ctx);
        cleanup_openssl();
    }
}
