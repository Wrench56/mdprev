#include <stdint.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdatomic.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "gen/gen.h"
#include "globals.h"
#include "notify/inotify.h"
#include "threadpool.h"

#define PROT_TCP 6

static const uint64_t evsig = 1;

typedef struct {
    int32_t cli_fd;
    _Atomic int32_t* evfd;
    const char* body;
} conn_handler_data_t;

static void conn_handler(void* payload) {
    conn_handler_data_t* data = (conn_handler_data_t*) payload;
    int32_t cli_fd = data->cli_fd;

    char recv_buf[8192];
    char headers[256];
    size_t body_len = strlen(data->body);

    read(cli_fd, recv_buf, sizeof(recv_buf));
    if (strncmp("GET / ", recv_buf, 6) == 0) {
        close(*data->evfd);
        *data->evfd = -1;

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

        if (write(cli_fd, headers, (size_t) hlen) < 0 ||
            write(cli_fd, data->body, body_len) < 0) {
            perror("send()");
        }
    } else if (strncmp("GET /event ", recv_buf, 11) == 0) {
        const char* sse_headers = "HTTP/1.1 200 OK\r\n"
                                  "Server: mdprev\r\n"
                                  "Content-Type: text/event-stream\r\n"
                                  "Cache-Control: no-cache\r\n"
                                  "Connection: keep-alive\r\n"
                                  "\r\n";

        if (write(cli_fd, sse_headers, strlen(sse_headers)) < 0) {
            perror("write()");
            goto cleanup;
        }

        const char* heartbeat = ": ping\n\n";
        const char* reload = "event: reload\ndata: now\n\n";

        struct itimerspec timer;
        timer.it_value.tv_sec = 1;
        timer.it_value.tv_nsec = 0;
        timer.it_interval.tv_sec = 1;
        timer.it_interval.tv_nsec = 0;

        uint64_t tmp;
        int32_t tfd = timerfd_create(CLOCK_MONOTONIC, 0);
        timerfd_settime(tfd, 0, &timer, NULL);

        struct pollfd fds[2] = {
            {
                .fd = *data->evfd,
                .events = POLLIN,
            },
            {
                .fd = tfd,
                .events = POLLIN,
            },
        };

        for (;;) {
            int32_t rc = poll(fds, 2, -1);
            if (rc == -1) {
                if (errno == EINTR) {
                    continue;
                }

                perror("poll()");
                break;
            }

            /* Timer event (ping) */
            if (fds[1].revents & POLLIN) {
                read(tfd, &tmp, sizeof(tmp));
                if (write(cli_fd, heartbeat, strlen(heartbeat)) < 0) {
                    break;
                }
            }

            /* File event (reload) */
            if (fds[0].revents & POLLIN) {
                read(*data->evfd, &tmp, sizeof(tmp));
                if (write(cli_fd, reload, strlen(reload)) < 0) {
                    break;
                }
            }
        }

        close(tfd);
        close(*data->evfd);
        *data->evfd = -1;
    } else {
        fprintf(stderr, "Warning: Unknown endpoint");
    }

cleanup:
    free(data);
    close(cli_fd);
}

void mdprev_host(uint16_t port) {
    md_to_html();

    signal(SIGPIPE, SIG_IGN);

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

    int yes = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt(SO_REUSEADDR)");
        close(sock_fd);
        return;
    }

    printf("Waiting...\n");
    bind(sock_fd, (const struct sockaddr*) &addr, sizeof(addr));
    if (listen(sock_fd, 1) == -1) {
        perror("bind()");
        close(sock_fd);
        return;
    }

    struct pollfd fds[2] = {
        {
            .fd = sock_fd,
            .events = POLLIN,
        },
        { .fd = track(), .events = POLLIN },
    };

    _Atomic int32_t evfds[THREADPOOL_SZ] = { -1 };
    for (size_t i = 0; i < THREADPOOL_SZ; i++) {
        evfds[i] = -1;
    }

    conn_handler_data_t* data = NULL;
    int32_t cli_fd = 0;
    char tmp[4096] = { 0 };

    for (;;) {
        /* Pre-allocate data struct for new connections */
        if (data == NULL) {
            data = malloc(sizeof(conn_handler_data_t));
            if (data == NULL) {
                perror("malloc()");
                exit(1);
            }
        }

        int32_t rc = poll(fds, 2, -1);
        if (rc == -1) {
            if (errno == EINTR) {
                continue;
            }

            perror("poll()");
            goto cleanup;
        }

        /* File event */
        if (fds[1].revents & POLLIN) {
            for (;;) {
                ssize_t n = read(fds[1].fd, tmp, sizeof(tmp));
                if (n > 0) {
                    continue;
                } else if (n == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    } else if (errno == EINTR) {
                        continue;
                    } else {
                        perror("inotify read");
                    }
                }

                break;
            }

            md_to_html();
            for (size_t i = 0; i < THREADPOOL_SZ; i++) {
                int32_t fd = evfds[i];
                if (fd != -1) {
                    write(fd, &evsig, sizeof(evsig));
                }
            }
        }

        /* New socket event */
        if (fds[0].revents & POLLIN) {
            cli_fd = accept(sock_fd, NULL, NULL);
            if (cli_fd == -1) {
                perror("accept()");
                close(sock_fd);
                return;
            }

            _Atomic int32_t* evfd_ptr = NULL;
            int32_t evfd = eventfd(0, EFD_NONBLOCK);
            for (size_t i = 0; i < THREADPOOL_SZ; i++) {
                if (evfds[i] == -1) {
                    evfds[i] = evfd;
                    evfd_ptr = &evfds[i];
                    break;
                }
            }

            if (evfd_ptr == NULL) {
                fprintf(stderr, "Error: no eventfd slots left\n");
                close(evfd);
                close(cli_fd);
                continue;
            }

            data->cli_fd = cli_fd;
            data->evfd = evfd_ptr;
            data->body = GENBODY;
            assign_worker(conn_handler, data);
            data = NULL;
        }
    }

cleanup:
    close(sock_fd);
    untrack();
}
