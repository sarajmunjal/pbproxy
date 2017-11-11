#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <poll.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>

#define TIMEOUT 0
#define BUFF_SIZE 4096

void error(char *str) {
    perror(str);
}

typedef struct program_args {
    char *dest_addr;
    unsigned int dest_port;
    unsigned int src_port;
    unsigned int is_server;
    char *key_file_path;
    int is_debug;
} args_t;

args_t *args;

args_t *parse_cli_arguments(int argc, char **argv) {
    args_t *args = (args_t *) malloc(sizeof(args_t));
    int c;
    opterr = 0;
    args->is_server = 0;
    args->is_debug = 0;
    while ((c = getopt(argc, argv, "k:l:d:")) != -1) {
        switch (c) {
            case 'l':
                args->src_port = atoi(optarg);
                args->is_server = 1;
                break;
            case 'd':
                args->is_debug = 1;
                break;
            case 'k':
                args->key_file_path = optarg;
                break;
            case '?':
                if (optopt == 'l' || optopt == 'k') {
                    fprintf(stderr, "Option -%c requires an argument. Char : %c\n", optopt, c);
                } else if (isprint(optopt)) {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                }
            default:
                abort();
        }
    }
    if (optind == argc) {
        perror("Must specify destination address and port.\n");
        return NULL;
    }
    args->dest_addr = argv[optind++];
    if (optind == argc) {
        perror("Must specify destination port.\n");
        return NULL;
    }
    args->dest_port = atoi(argv[optind]);
    return args;
}


ssize_t write_to_socket(FILE *ofp, struct pollfd *sock_poll_fd, int dest_sock_fd, char *data, ssize_t data_len) {
    int rv = poll(sock_poll_fd, 1, TIMEOUT);
    if (rv == -1) {
        perror("poll write error"); // error occurred in poll()
        return -1;
    }
    if (rv == 0) {
        return -1;
    }

    // ready for write:
    if ((*sock_poll_fd).revents & POLLOUT) {
        return write(dest_sock_fd, data, data_len); // write
    }
    // error in polling
//            if (send_ufd.revents & POLL_ERR) {
//                perror("Error occurred while polling forward socket for write");
//                break;
//            }
//
    // check for poll hung up
//            if (send_ufd.revents & POLL_HUP) {
//                perror("Connection hung up while polling forward socket for write");
//                break;
//            }
    return -1;
}

typedef struct f2s_command {
    FILE *ifp;
    struct pollfd *pollfd;
    int sock_fd;
    int *conn_broken;
    FILE *ofp;
} f2s_cmd_t;

ssize_t read_fd(char *buf, size_t size, int fd) {
    return read(fd, buf, size);
}

void client_file_to_socket(f2s_cmd_t *cmd) {
    char buffer[BUFF_SIZE];
    while (1) {
        bzero(buffer, BUFF_SIZE);
        fflush(cmd->ofp);
        // block for stdin
        ssize_t read_count = read_fd(buffer, BUFF_SIZE - 1, fileno(cmd->ifp));
        if (read_count < 0 && errno == EAGAIN) {
            continue;
        }
        if (read_count == 0) {
            continue;
        }
        if (read_count < 0) {
            (*cmd->conn_broken) = 1;
            break;
        }
        if (args->is_debug) {
            fprintf(cmd->ofp, "send: %s", buffer);
        }
        // wait for write ready
        ssize_t written_len = write_to_socket(cmd->ofp, cmd->pollfd, cmd->sock_fd, buffer, read_count);
        if (written_len < 0) {
            (*cmd->conn_broken) = 1;
            break;
        }
    }
}

int main(int argc, char **argv) {
    args = parse_cli_arguments(argc, argv);
    if (args == NULL) {
        perror("Some error occurred with input");
        return -2;
    }
    int fwd_sock_fd, portno;
    struct sockaddr_in fwd_serv_addr;
    FILE *ifp = stdin;
    FILE *ofp = !(args->is_debug) ? stderr : fopen(args->is_server ? "./logs/server-log.txt" : "./logs/client-log.txt",
                                                   "w");
    portno = args->dest_port;
    char buffer[BUFF_SIZE];
    fwd_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    char *hostname = args->dest_addr;
    struct hostent *server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "Couldn't find any host with hostname: %s", hostname);
        return -1;
    }
    bzero((char *) &fwd_serv_addr, sizeof(fwd_serv_addr));
    fwd_serv_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr,
          (char *) &fwd_serv_addr.sin_addr.s_addr,
          server->h_length);
    fwd_serv_addr.sin_port = htons(portno);
    if (connect(fwd_sock_fd, (struct sockaddr *) &fwd_serv_addr, sizeof(fwd_serv_addr)) < 0) {
        error("ERROR connecting to server");
        return 0;
    }
    struct pollfd fwd_recv_ufd, fwd_send_ufd;
    fwd_recv_ufd.fd = fwd_sock_fd;
    fwd_recv_ufd.events = POLLIN | POLLPRI;

    fwd_send_ufd.fd = fwd_sock_fd;
    fwd_send_ufd.events = POLLOUT;
    bzero(buffer, BUFF_SIZE);
    if (args->is_server) {
        struct sockaddr_in in_client_addr;
        int client_sock_fd;
        if ((client_sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Error while creating client socket file descriptor");
            exit(1);
        }
        int true = 1;
        if (setsockopt(client_sock_fd, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(int)) == -1) {
            perror("Error in Setsockopt for client socket");
            exit(1);
        }
        in_client_addr.sin_family = AF_INET;
        in_client_addr.sin_port = htons(args->src_port);
        in_client_addr.sin_addr.s_addr = INADDR_ANY;
        bzero(&(in_client_addr.sin_zero), 8);
        if (bind(client_sock_fd, (struct sockaddr *) &in_client_addr, sizeof(struct sockaddr)) == -1) {
            perror("Unable to bind incoming client socket with client file descriptor");
            exit(1);
        }

        while (1) {
            if (args->is_debug) {
                fprintf(ofp, "\nServer waiting for client on port %d", args->src_port);
            }
            if (listen(client_sock_fd, 5) == -1) {
                perror("Error while trying to listen for client socket connection");
                exit(1);
            }
            fflush(ofp);
            size_t sin_size = sizeof(struct sockaddr_in);
            int connected_cli_socket_fd = accept(client_sock_fd, (struct sockaddr *) &in_client_addr, &sin_size);
            if (args->is_debug) {
                fprintf(ofp, "\n I got a connection from (%s , %d)",
                        inet_ntoa(in_client_addr.sin_addr), ntohs(in_client_addr.sin_port));
                fflush(ofp);
            }

            struct pollfd in_client_rcv_ufd, in_client_send_ufd;
            in_client_rcv_ufd.fd = connected_cli_socket_fd;
            in_client_rcv_ufd.events = POLLIN | POLLPRI;

            in_client_send_ufd.fd = connected_cli_socket_fd;
            in_client_send_ufd.events = POLLOUT;
            while (1) {
                bzero(buffer, BUFF_SIZE);
                int rv = poll(&in_client_rcv_ufd, 1, TIMEOUT);

                if (rv == -1) {
                    perror("Error occurred in polling incoming client socket for reading."); // error occurred in poll()
                } else if (rv == 0) {
//                    printf("Timeout occurred!  No incoming data read from client after 3.5 seconds.\n");
                } else {
                    // check for events on s1:
                    if (in_client_rcv_ufd.revents & POLLIN) {
                        ssize_t nrecv = recv(connected_cli_socket_fd, buffer, BUFF_SIZE - 1, 0); // receive normal data
                        if (nrecv < 0) {
                            break;
                        }
                        write_to_socket(ofp, &fwd_send_ufd, fwd_sock_fd, buffer, nrecv);
                    }
                    if (in_client_rcv_ufd.revents & POLLPRI) {
                        ssize_t nrecv = recv(connected_cli_socket_fd, buffer, BUFF_SIZE - 1, MSG_OOB);
                        if (nrecv < 0) {
                            break;
                        }
                        write_to_socket(ofp, &fwd_send_ufd, fwd_sock_fd, buffer, nrecv);
                    }

//            // error in polling
//            if (send_ufd.revents & POLL_ERR) {
//                perror("Error occurred while polling forward socket for read");
//                break;
//            }
                    // check for poll hung up
//                    if (in_client_rcv_ufd.revents & POLL_HUP) {
//                        perror("Connection hung up while polling forward socket for read");
//                        break;
//                    }
                }

                bzero(buffer, BUFF_SIZE);
                // wait for message from forward socket
                rv = poll(&fwd_recv_ufd, 1, TIMEOUT);

                if (rv == -1) {
                    perror("Error occurred in polling forward server socket for reading."); // error occurred in poll()
                } else if (rv == 0) {
//                    printf("Timeout occurred!  No incoming data read from forward server after 3.5 seconds.\n");
                } else {
                    // check for events on s1:
                    if (fwd_recv_ufd.revents & POLLIN) {
                        ssize_t nrecv = recv(fwd_sock_fd, buffer, BUFF_SIZE - 1, 0); // receive normal data
                        if (nrecv < 0) {
                            break;
                        }
                        write_to_socket(ofp, &in_client_send_ufd, connected_cli_socket_fd, buffer, nrecv);
                    }
                    if (fwd_recv_ufd.revents & POLLPRI) {
                        ssize_t nrecv = recv(fwd_sock_fd, buffer, BUFF_SIZE - 1, MSG_OOB); // out-of-band data
                        if (nrecv < 0) {
                            break;
                        }
                        write_to_socket(ofp, &in_client_send_ufd, connected_cli_socket_fd, buffer, nrecv);
                    }

//            // error in polling
//            if (send_ufd.revents & POLL_ERR) {
//                perror("Error occurred while polling forward socket for read");
//                break;
//            }
                    // check for poll hung up
//            if (send_ufd.revents & POLL_HUP) {
//                perror("Connection hung up while polling forward socket for read");
//                break;
//            }
                }

            }
        }
    } else {
        pthread_t client_write_thread;
        f2s_cmd_t cmd;
        int conn_broken = 0;
        cmd.ifp = ifp;
        cmd.ofp = ofp;
        cmd.sock_fd = fwd_sock_fd;
        cmd.pollfd = &fwd_send_ufd;
        cmd.conn_broken = &conn_broken;
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK)) {
            perror("Failed to make stdin non-blocking\n");
            return -2;
        }
        if (pthread_create(&client_write_thread, NULL, client_file_to_socket, &cmd) != 0) {
            perror("Error creating separate client writing thread: ");
            return -2;
        }
        while (1) {
            if (conn_broken) {
                break;
            }
            bzero(buffer, BUFF_SIZE);
            int rv = poll(&fwd_recv_ufd, 1, TIMEOUT);

            if (rv == -1) {
                perror("read poll error"); // error occurred in poll()
            } else if (rv == 0) {
//                fprintf(f"Timeout occurred!  No read data after 3.5 seconds.\n");
            } else {
                if (fwd_recv_ufd.revents & POLLIN) {
                    ssize_t nrecv = recv(fwd_sock_fd, buffer, BUFF_SIZE - 1, 0); // receive normal data
                    if (nrecv <= 0) {
                        break;
                    }
                    write(STDOUT_FILENO, buffer, nrecv);
                    if (args->is_debug) {
                        fprintf(ofp, "rcv: %s", buffer);
                    }
                }
                if (fwd_recv_ufd.revents & POLLPRI) {
                    ssize_t nrecv = recv(fwd_sock_fd, buffer, BUFF_SIZE - 1, MSG_OOB); // out-of-band data
                    if (nrecv <= 0) {
                        break;
                    }
                    write(STDOUT_FILENO, buffer, nrecv);
                    if (args->is_debug) {
                        fprintf(ofp, "rcv: %s", buffer);
                    }
                }
            }
        }
        pthread_join(client_write_thread, NULL);
        if (conn_broken) {
            fprintf(stderr, "Connection with server was broken");
        }
    }

    close(fwd_sock_fd);
    return 0;
}