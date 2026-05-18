#include <stdint.h>

#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PROT_TCP 6

void mdprev_host(uint16_t port) {
    int32_t sock_fd = socket(AF_INET, SOCK_STREAM, PROT_TCP);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = INADDR_ANY
    };

    if (sock_fd == -1) {
        perror("socket()");
        return;
    }

    printf("Waiting...\n");
    bind(sock_fd, (const struct sockaddr*) &addr, sizeof(addr));
    if (listen(sock_fd, 1) == -1) {
        perror("bind()");
        close(sock_fd);
        return;
    }

    int32_t cli_fd = accept(sock_fd, NULL, NULL);
    if (cli_fd == -1) {
        perror("accept()");
        close(sock_fd);
        return;
    }

    const char body[] = "<h1>Hello</h1>";

    char buf[512];
    int len = snprintf(
        buf, sizeof(buf),
        "HTTP/1.1 200 OK\r\n"
        "Server: mdprev\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        strlen(body), body
    );

    if (len < 0 || (size_t) len >= sizeof(buf)) {
        fprintf(stderr, "Error: Response buffer too small!\n");
        goto cleanup;
    }

    if (send(cli_fd, buf, (size_t) len, 0) == -1) {
        perror("send()");
    }

cleanup:
    close(cli_fd);
    close(sock_fd);
}
