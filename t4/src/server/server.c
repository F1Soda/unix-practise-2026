#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>

#include "logger.h"

#define MAX_SOCKET_NAME_LEN 256
#define MAX_CLIENTS 100
#define BUFFER_SIZE 16

typedef struct {
    char buf[BUFFER_SIZE];
    int bytes_in_buf;
} ClientState;

typedef struct {
    int accepted_connections;
    int rejected_connections;
} Statistics;

static volatile sig_atomic_t shutdown_flag = 0;

void handle_shutdown(int sig) {
    shutdown_flag = 1;
}

static char *get_socket_name_from_config(FILE *config_file, char *out_buf, const int buf_size) {
    if (fgets(out_buf, buf_size, config_file) != NULL) {
        out_buf[strcspn(out_buf, "\r\n")] = '\0';
        return out_buf;
    }

    log_perror("Failed to parse socket name from config");
    return NULL;
}

static int close_socket(const int sock, const char *socket_filepath) {
    int res = 0;

    if (close(sock) != 0) {
        log_perror("Failed to close socket");
        res = 1;
    }

    if (unlink(socket_filepath) != 0) {
        log_perror("Failed to unlink socket file");
        res = 1;
    }

    return res;
}

static int setup_socket(const char *socket_file_path) {
    if (unlink(socket_file_path) == -1 && errno != ENOENT) {
        log_perror("Failed to unlink old socket file %s", socket_file_path);
    }

    const int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        log_perror("Failed to create socket");
        return -1;
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_file_path, sizeof(addr.sun_path) - 1);

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        log_perror("Failed bind socket to %s", socket_file_path);
        close(sock);
        return -1;
    }

    if (listen(sock, SOMAXCONN) != 0) {
        log_perror("Failed to listen socket");
        close_socket(sock, socket_file_path);
        return -1;
    }

    log_info("Socket successfully created and binded to file: %s", socket_file_path);
    return sock;
}

static int setup_sigactions() {
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sa.sa_handler = handle_shutdown;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        log_error("Failed to setup SIGINT handler");
        return 1;
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        log_error("Failed to setup SIGTERM handler");
        return 1;
    }

    return 0;
}

static void handle_new_connection(const int listen_sock, struct pollfd *fds, ClientState *client_states, int *nfds) {
    const int client_fd = accept(listen_sock, NULL, NULL);
    if (client_fd == -1) {
        log_perror("Failed to accept connection");
        return;
    }

    if (*nfds >= MAX_CLIENTS + 1) {
        log_warn("Server full, closing connection");
        close(client_fd);
        return;
    }

    fds[*nfds].fd = client_fd;
    fds[*nfds].events = POLLIN;

    memset(&client_states[*nfds], 0, sizeof(ClientState));

    (*nfds)++;

    void *current_heap_end = sbrk(0);

    log_info("New client connected: fd=%d, heap_end=%p, total_clients=%d",
             client_fd, current_heap_end, *nfds - 1);
}

static void handle_close_client(const int client, struct pollfd fds[], ClientState client_states[], int *nfds) {
    if (close(fds[client].fd) != 0) {
        log_pwarn("Failed to close client connection %d fd", fds[client].fd);
    }

    client_states[client].bytes_in_buf = 0;
    client_states[client].buf[0] = '\0';

    // Сдвигаем последний элемент на место удаляемого, если это не один и тот же элемент
    if (client < *nfds - 1) {
        fds[client] = fds[*nfds - 1];
        client_states[client] = client_states[*nfds - 1];
    }
    (*nfds)--;

    void *current_heap_end = sbrk(0);
    log_info("Client disconnected: fd=%d, heap_end=%p, total_clients=%d",
         fds[client].fd, current_heap_end, *nfds - 1);
}

static int handle_client_data(const int client, struct pollfd fds[], ClientState client_states[], int *state,
                              int *nfds) {
    ClientState *cs = &client_states[client];

    const ssize_t n = read(fds[client].fd, cs->buf + cs->bytes_in_buf, BUFFER_SIZE - cs->bytes_in_buf - 1);

    if (n <= 0) {
        if (n < 0) log_perror("Read error from %d fd client", fds[client].fd);
        handle_close_client(client, fds, client_states, nfds);
        return 1;
    }

    cs->bytes_in_buf += n;
    cs->buf[cs->bytes_in_buf] = '\0';

    char *newlineptr = strchr(cs->buf, '\n');
    if (newlineptr) {
        *newlineptr = '\0';

        char *endptr;
        errno = 0;
        long val = strtol(cs->buf, &endptr, 10);

        if (endptr == cs->buf || *endptr != '\0') {
            log_warn("Invalid input from client %d fd: '%s'", fds[client].fd, cs->buf);
        } else if (errno == ERANGE) {
            log_warn("Number out of range from client %d fd", fds[client].fd);
        } else {
            *state += (int) val;
            log_info("Received %d from %d fd client, new state: %d", val, fds[client].fd, *state);
        }

        char resp[32];
        const int resp_len = snprintf(resp, sizeof(resp), "%d\n", *state);

        if (send(fds[client].fd, resp, resp_len, 0) < 0) {
            log_perror("Send error from client %d fd", fds[client].fd);
        }

        int processed_len = (newlineptr - cs->buf) + 1;
        int remaining = cs->bytes_in_buf - processed_len;
        if (remaining > 0) {
            memmove(cs->buf, newlineptr + 1, remaining);
        }
        cs->bytes_in_buf = remaining;
    } else if (cs->bytes_in_buf >= BUFFER_SIZE - 1) {
        log_warn("Buffer overflow from client %d fd, clearing", fds[client].fd);
        cs->bytes_in_buf = 0;
    }

    return 0;
}

static int server_loop(const int listen_sock) {
    struct pollfd fds[MAX_CLIENTS + 1];
    ClientState client_states[MAX_CLIENTS + 1];
    int nfds = 1;
    int state = 0;

    memset(fds, 0, sizeof(fds));
    fds[0].fd = listen_sock;
    fds[0].events = POLLIN;

    log_info("Starting server. pool size: %d", MAX_CLIENTS);

    while (!shutdown_flag) {
        int ret = poll(fds, nfds, -1);
        if (ret < 0) {
            if (errno == EINTR) {
                log_info("Poll interrupted by signal");
                break;
            }

            log_perror("Failed to poll sockets");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == listen_sock) {
                    handle_new_connection(listen_sock, fds, client_states, &nfds);
                } else {
                    const int is_client_disconnected = handle_client_data(i, fds, client_states, &state, &nfds);
                    if (is_client_disconnected) {
                        i--;
                    }
                }
            }
        }
    }

    log_info("Stopping server");
    // 1 fd — сокет открытый сервером. Он будет закрыт позже
    for (int i = 1; i < nfds; i++) {
        if (close(fds[i].fd) != 0) {
            log_perror("Failed to close client socket");
        } else {
            log_info("Closed client %d, %d", i, fds[i].fd);
        }
    }

    log_info("Server stopped");

    return 0;
}


int run_server(const char *config_file_path) {
    if (setup_sigactions() != 0) {
        return 1;
    }

    FILE *config_file = fopen(config_file_path, "r");
    if (config_file == NULL) {
        log_perror("Failed to open config file");
        return 1;
    }

    char buffer[MAX_SOCKET_NAME_LEN];
    const char *socket_name = get_socket_name_from_config(config_file, buffer, MAX_SOCKET_NAME_LEN);

    if (fclose(config_file) != 0) {
        log_perror("Failed to close config file");
        return 1;
    }

    if (socket_name == NULL) {
        return 1;
    }

    char socket_file_path[MAX_SOCKET_NAME_LEN + 5]; // "/tmp/" + имя файла
    snprintf(socket_file_path, sizeof(socket_file_path), "/tmp/%s", socket_name);

    const int sock = setup_socket(socket_file_path);
    if (sock == -1) {
        return 1;
    }

    int res = server_loop(sock);

    if (close_socket(sock, socket_file_path) != 0) {
        res = 1;
    }

    return res;
}
