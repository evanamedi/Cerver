#ifndef UTILS_H
#define UTILS_H

void log_message(const char *client_ip, 
				 const char *request_method, 
				 const char *request_uri, 
				 int response_status, 
				 int print_to_console);

#endif