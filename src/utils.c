#include <stdio.h>
#include <time.h>
#include <string.h>
#include "utils.h"

#define LOG_FILE "server.log"
#define MAX_LOG_SIZE 1024 * 1024  // 1 MB

void rotate_logs() {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        perror("fopen");
        return;
    }

    fseek(log_file, 0, SEEK_END);
    long file_size = ftell(log_file);
    fclose(log_file);

    if (file_size >= MAX_LOG_SIZE) {
        char old_log[64];
        time_t now = time(NULL);
        strftime(old_log, sizeof(old_log), "server.log.%Y%m%d%H%M%S", localtime(&now));

        if (rename(LOG_FILE, old_log) != 0) {
            perror("rename");
        }
    }
}

void log_message(const char *client_ip, const char *request_method, const char *request_uri, int response_status, int print_to_console) {
    rotate_logs();

    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        perror("fopen");
        return;
    }

    time_t now = time(NULL);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(log_file, "[%s] %s %s %s %d\n", time_str, client_ip, request_method, request_uri, response_status);
    fclose(log_file);

    if (print_to_console) {
        printf("[%s] %s %s %s %d\n", time_str, client_ip, request_method, request_uri, response_status);
    }
}