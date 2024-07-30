#include "unity.h"
#include "mime.h"
#include "utils.h"
#include "request_handler.h"
#include "ssl_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

// Setup and Teardown functions
void setUp(void) {
    // Create test directory and files
    mkdir("test_files", 0777);
    mkdir("test_files/www", 0777);

    // Create a test index.html file
    FILE *file = fopen("test_files/www/index.html", "w");
    fprintf(file, "<html><body><h1>Testing Page</h1></body></html>");
    fclose(file);

    // Ensure the test log file is clean
    remove("test_server.log");
}

void tearDown(void) {
    // Remove test files and directory
    remove("test_files/www/index.html");
    rmdir("test_files/www");
    rmdir("test_files");

    // Remove test log file if it exists
    remove("test_server.log");
}

// MIME Type Detection Tests
void test_get_mime_type(void) {
    TEST_ASSERT_EQUAL_STRING("text/html", get_mime_type("index.html"));
    TEST_ASSERT_EQUAL_STRING("text/css", get_mime_type("style.css"));
    TEST_ASSERT_EQUAL_STRING("application/javascript", get_mime_type("script.js"));
    TEST_ASSERT_EQUAL_STRING("image/png", get_mime_type("image.png"));
    TEST_ASSERT_EQUAL_STRING("image/jpeg", get_mime_type("image.jpg"));
    TEST_ASSERT_EQUAL_STRING("application/octet-stream", get_mime_type("unknown"));
    TEST_ASSERT_EQUAL_STRING("application/octet-stream", get_mime_type("file."));
}

// Log Message Function Tests
void test_log_message(void) {
    // Log a message and check the content
    log_message("127.0.0.1", "GET", "/index.html", 200, 0);

    FILE *log_file = fopen("test_server.log", "r");
    TEST_ASSERT_NOT_NULL(log_file);

    char buffer[256];
    fgets(buffer, sizeof(buffer), log_file);
    fclose(log_file);

    // Check if the log entry contains expected content
    TEST_ASSERT_NOT_NULL(strstr(buffer, "127.0.0.1"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "GET"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "/index.html"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "200"));
}

// Function to simulate a client request for request handler tests
void simulate_client_request(const char *request, char *response, size_t response_size) {
    int pipe_fd[2];
    pipe(pipe_fd);

    // Simulate writing the request to the client socket
    write(pipe_fd[1], request, strlen(request));
    close(pipe_fd[1]);

    // Call the request handler with the simulated client socket
    handle_client_request(pipe_fd[0], "127.0.0.1", NULL);

    // Read the response from the simulated client socket
    ssize_t bytes_read = read(pipe_fd[0], response, response_size - 1);
    if (bytes_read >= 0) {
        response[bytes_read] = '\0';  // Null-terminate the response
    }
    close(pipe_fd[0]);

    // Print response for debugging
    printf("Response:\n%s\n", response);
}

// Function to simulate an HTTPS client request for request handler tests
void simulate_https_client_request(const char *request, char *response, size_t response_size) {
    SSL_CTX *ctx;
    SSL *ssl;
    int server = -1;
    struct sockaddr_in addr;
    int port = 8080;

    initialize_openssl();
    ctx = create_context();
    configure_context(ctx);

    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        perror("Unable to create socket");
        exit(EXIT_FAILURE);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(server, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Unable to connect");
        exit(EXIT_FAILURE);
    }

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, server);

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
    } else {
        SSL_write(ssl, request, strlen(request));
        ssize_t bytes_read = SSL_read(ssl, response, response_size - 1);
        if (bytes_read >= 0) {
            response[bytes_read] = '\0';  // Null-terminate the response
        }
        SSL_shutdown(ssl);
    }

    SSL_free(ssl);
    close(server);
    SSL_CTX_free(ctx);
    cleanup_openssl();
}

// Client Request Handling Tests
void test_handle_client_request(void) {
    char response[8192];

    // Test existing file
    simulate_client_request("GET /index.html HTTP/1.1\r\n\r\n", response, sizeof(response));
    TEST_ASSERT_NOT_NULL(strstr(response, "HTTP/1.1 200 OK"));

    // Test non-existing file
    simulate_client_request("GET /nonexistent.html HTTP/1.1\r\n\r\n", response, sizeof(response));
    TEST_ASSERT_NOT_NULL(strstr(response, "HTTP/1.1 404 Not Found"));

    // Test directory request
    simulate_client_request("GET / HTTP/1.1\r\n\r\n", response, sizeof(response));
    TEST_ASSERT_NOT_NULL(strstr(response, "HTTP/1.1 200 OK"));
}

// HTTPS Client Request Handling Tests
void test_handle_https_client_request(void) {
    char response[8192];

    // Test existing file over HTTPS
    simulate_https_client_request("GET /index.html HTTP/1.1\r\n\r\n", response, sizeof(response));
    TEST_ASSERT_NOT_NULL(strstr(response, "HTTP/1.1 200 OK"));

    // Test non-existing file over HTTPS
    simulate_https_client_request("GET /nonexistent.html HTTP/1.1\r\n\r\n", response, sizeof(response));
    TEST_ASSERT_NOT_NULL(strstr(response, "HTTP/1.1 404 Not Found"));

    // Test directory request over HTTPS
    simulate_https_client_request("GET / HTTP/1.1\r\n\r\n", response, sizeof(response));
    TEST_ASSERT_NOT_NULL(strstr(response, "HTTP/1.1 200 OK"));
}

// Main function to run tests
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_get_mime_type);
    RUN_TEST(test_log_message);
    RUN_TEST(test_handle_client_request);
    RUN_TEST(test_handle_https_client_request);
    return UNITY_END();
}