#include "server.h"

int main() {
    int port = 8080;
    int use_https = 1; // 1 to use HTTPS, 0 for HTTP
    start_server(port, use_https);
    return 0;
}
