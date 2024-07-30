#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "request_handler.h"
#include "mime.h"
#include "utils.h"

#define RESPONSE_BUFFER_SIZE 8192

void handle_client_request(int client_socket, const char *client_ip, SSL *ssl) {
    char buffer[1024] = {0};
    char response[RESPONSE_BUFFER_SIZE] = {0};
    char request_method[8];
    char request_uri[1024];
    int response_status = 200;
    int read_val;

    if (ssl) {
        read_val = SSL_read(ssl, buffer, sizeof(buffer));
    } else {
        read_val = read(client_socket, buffer, sizeof(buffer));
    }

    if (read_val < 0) {
        perror("read");
        if (ssl) SSL_shutdown(ssl);
        close(client_socket);
        return;
    } else if (read_val == 0) {
        if (ssl) SSL_shutdown(ssl);
        close(client_socket);
        return;
    }

    sscanf(buffer, "%s %s", request_method, request_uri);

    if (strcmp(request_method, "GET") != 0) {
        response_status = 405;
        snprintf(response, sizeof(response), "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n");
        if (ssl) {
            SSL_write(ssl, response, strlen(response));
        } else {
            write(client_socket, response, strlen(response));
        }
        log_message(client_ip, request_method, request_uri, response_status, 1);
        if (ssl) SSL_shutdown(ssl);
        close(client_socket);
        return;
    }

    char file_path[1024] = "www";
    strcat(file_path, request_uri);

    if (file_path[strlen(file_path) - 1] == '/') {
        strcat(file_path, "index.html");
    }

    int file_fd = open(file_path, O_RDONLY);
    if (file_fd == -1) {
        response_status = 404;
        snprintf(response, sizeof(response), "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
        if (ssl) {
            SSL_write(ssl, response, strlen(response));
        } else {
            write(client_socket, response, strlen(response));
        }
        log_message(client_ip, request_method, request_uri, response_status, 1);
        if (ssl) SSL_shutdown(ssl);
        close(client_socket);
        return;
    }

    struct stat file_stat;
    fstat(file_fd, &file_stat);

    const char *mime_type = get_mime_type(file_path);
    snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %lld\r\n\r\n", mime_type, (long long)file_stat.st_size);

    if (ssl) {
        SSL_write(ssl, response, strlen(response));
    } else {
        write(client_socket, response, strlen(response));
    }

    int bytes_read;
    while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
        if (ssl) {
            SSL_write(ssl, buffer, bytes_read);
        } else {
            write(client_socket, buffer, bytes_read);
        }
    }

    close(file_fd);
    log_message(client_ip, request_method, request_uri, response_status, 1);
    if (ssl) SSL_shutdown(ssl);
    close(client_socket);
}
