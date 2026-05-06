#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <getopt.h>

#define MAX_SOCKET_NAME_LEN 256

static volatile sig_atomic_t shutdown_flag = 0;

void handle_shutdown(int sig) {
    shutdown_flag = 1;
}

static char *get_socket_name_from_config(FILE *config_file, char *out_buf, const int buf_size) {
    if (fgets(out_buf, buf_size, config_file) != NULL) {
        out_buf[strcspn(out_buf, "\r\n")] = '\0';
        return out_buf;
    }

    perror("Failed to parse socket name from config");
    return NULL;
}

static int setup_sigactions() {
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = handle_shutdown;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        fprintf(stderr, "Failed to setup SIGINT handler");
        return 1;
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        fprintf(stderr, "Failed to setup SIGTERM handler");
        return 1;
    }

    return 0;
}

static int setup_socket(const char *socket_file_path) {
    const int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Failed to create socket");
        return -1;
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_file_path, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
        perror("client sock connect");
        fprintf(stderr, "Failed to connect to server. Socket: %s\n", socket_file_path);
        close(sock);
        return -1;
    }

    printf("Successfully connected to socket: %s\n", socket_file_path);
    return sock;
}

static int send_and_print(const int sock, const char *buf, const size_t len) {
    if (len == 0) {
        return 0;
    }

    if (send(sock, buf, len, 0) == -1) {
        return 1;
    }

    char recv_buf[32];
    const ssize_t n = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
    if (n <= 0) {
        if (n == 0) printf("Server closed connection\n");
        else perror("Recv failed");
        return 1;
    }

    recv_buf[n] = '\0';
    printf("Server response: %s", recv_buf);

    return 0;
}

static int save_stats(const char *stats_file_path, const long long total_client_delay) {
    FILE *stats_file = fopen(stats_file_path, "a");
    if (stats_file == NULL) {
        perror("Failed to open data file");
        return 1;
    }

    int res = 0;

    if (fprintf(stats_file, "%lld\n", total_client_delay) < 0) {
        perror("Failed to write data file");
        res = 1;
    }


    if (fclose(stats_file) != 0) {
        perror("Failed to close stats file");
        res = 1;
    }

    return res;
}

static int run_test_loop(const int sock, const char *data_file_path, const int delay_ms, const char *stats_file_path) {
    srand(time(NULL) ^ getpid());
    int res = 0;

    FILE *data_file = fopen(data_file_path, "r");
    if (data_file == NULL) {
        perror("Failed to open data file");
        return 1;
    }

    char buffer[64];
    int buf_idx = 0;

    int bytes_to_next_pause = (rand() % 255) + 1;
    int bytes_read_total = 0;
    long long total_client_delay = 0;

    while (!shutdown_flag) {
        const int c = fgetc(data_file);

        if (c == EOF) {
            break;
        }

        buffer[buf_idx++] = (char) c;
        bytes_read_total++;

        if (c == '\n') {
            if (send_and_print(sock, buffer, buf_idx) != 0) {
                res = 1;
                break;
            }
            buf_idx = 0;
        }

        // задержка через каждые 1-255 байт
        if (bytes_read_total >= bytes_to_next_pause) {
            int time_to_sleep_ms = delay_ms;
            if (delay_ms < 0) {
                time_to_sleep_ms = (rand() % 300) + 100; // 0.1 - 0.4 сек
            }

            usleep(time_to_sleep_ms * 1000);
            total_client_delay += time_to_sleep_ms;
            bytes_to_next_pause = bytes_read_total + (rand() % 255) + 1;
        }
    }

    if (stats_file_path != NULL) {
        if (save_stats(stats_file_path, total_client_delay) != 0) {
            res = 1;
        }
    }


    if (fclose(data_file) != 0) {
        perror("Failed to close data file");
        res = 1;
    }

    return res;
}

static int run_loop(const int sock) {
    char send_buf[16];

    printf("Enter numbers (max 10 chars): \n");

    while (!shutdown_flag) {
        fflush(stdout);

        if (fgets(send_buf, sizeof(send_buf), stdin) == NULL) {
            break; // EOF
        }

        if (send_and_print(sock, send_buf, strlen(send_buf)) != 0) {
            break;
        }
    }

    printf("Disconnecting\n");

    return 0;
}

int main(const int argc, char **argv) {
    char *config_file_path = NULL;
    char *data_file_path = NULL;
    char *stats_file_path = NULL;
    int delay_ms = -1;

    static struct option long_options[] = {
        {"config", required_argument, 0, 1},
        {"data", optional_argument, 0, 2},
        {"stats", optional_argument, 0, 3},
        {"delay", optional_argument, 0, 4},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "", long_options, NULL)) != -1) {
        switch (opt) {
            case 1:
                config_file_path = optarg;
                break;
            case 2:
                data_file_path = optarg;
                break;
            case 3:
                stats_file_path = optarg;
                break;
            case 4:
                if (optarg == NULL) {
                    delay_ms = -1;
                }
                else {
                    char *endptr;
                    const long val = strtol(optarg, &endptr, 10);

                    if (optarg == endptr || *endptr != '\0') {
                        fprintf(stderr, "Error: --delay is not number: %s\n", optarg);
                        return 1;
                    }

                    delay_ms = (int) val;
                }
                break;
            default:
                return 1;
        }
    }

    if (config_file_path == NULL) {
        fprintf(stderr, "Usage: %s --config=<path> [--data=<path>] [--stats=<patn>] [--delay=<ms>]\n", argv[0]);
        return 1;
    }

    if (setup_sigactions() != 0) {
        return 1;
    }

    FILE *config_file = fopen(config_file_path, "r");
    if (config_file == NULL) {
        perror("Failed to open config file");
        return 1;
    }

    char buffer[MAX_SOCKET_NAME_LEN];
    const char *socket_name = get_socket_name_from_config(config_file, buffer, MAX_SOCKET_NAME_LEN);

    if (fclose(config_file) != 0) {
        perror("Failed to close config file");
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

    int res = data_file_path != NULL ? run_test_loop(sock, data_file_path, delay_ms, stats_file_path) : run_loop(sock);

    if (close(sock) != 0) {
        perror("Failed to close socket");
        res = 1;
    }

    return res;
}
