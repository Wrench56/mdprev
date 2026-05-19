#include <stdint.h>

#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define PROT_TCP 6

void mdprev_host(uint16_t port, const char* body) {
    int32_t sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
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

    int32_t cli_fd = 0;
    char headers[256];
    size_t body_len = strlen(body);
    for (;;) {
        cli_fd = accept(sock_fd, NULL, NULL);
        if (cli_fd == -1) {
            perror("accept()");
            close(sock_fd);
            return;
        }

        int32_t hlen = snprintf(
            headers,
            sizeof(headers),
            "HTTP/1.1 200 OK\r\n"
            "Server: mdprev\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n",
            body_len
        );

        if (hlen < 0 || (size_t) hlen >= sizeof(headers)) {
            fprintf(stderr, "Error: header buffer too small!\n");
            goto cleanup;
        }

        if (send(cli_fd, headers, (size_t) hlen, 0) < 0 ||
            send(cli_fd, body, body_len, 0) < 0) {
            perror("send()");
        }
    }

cleanup:
    close(cli_fd);
    close(sock_fd);
}
