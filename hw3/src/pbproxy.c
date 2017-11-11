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

void error(char *str) {
    perror(str);
}

typedef struct program_args {
    char *dest_addr;
    unsigned int dest_port;
    unsigned int src_port;
    unsigned int is_server;
    char *key_file_path;
} args_t;

args_t *parse_cli_arguments(int argc, char **argv) {
    args_t *args = (args_t *) malloc(sizeof(args_t));
    int c;
    opterr = 0;
    args->is_server = 0;
    while ((c = getopt(argc, argv, "k:l:")) != -1) {
        switch (c) {
            case 'l':
                args->src_port = atoi(optarg);
                args->is_server = 1;
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

int main(int argc, char **argv) {
    args_t *args = parse_cli_arguments(argc, argv);
    if (args == NULL) {
        perror("Some error occurred with input");
        return -2;
    }
    int fwd_sock_fd, portno, n;
    struct sockaddr_in serv_addr;
    FILE *ifp = stdin;
    FILE *ofp = stdout;
    portno = args->dest_port;
    char buffer[1024];
    fwd_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    char *hostname = args->dest_addr;
    struct hostent *server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(ofp, "Couldn't find any host with hostname: %s", hostname);
        return -1;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr,
          (char *) &serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(fwd_sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR connecting to server");
        return 0;
    }
    struct pollfd recv_ufd, send_ufd;
    recv_ufd.fd = fwd_sock_fd;
    recv_ufd.events = POLLIN | POLLPRI;

    send_ufd.fd = fwd_sock_fd;
    send_ufd.events = POLLOUT;
    fprintf(ofp, "Please enter the message: ");
    bzero(buffer, 1024);

    while (1) {
        bzero(buffer, 1024);
        // block for stdin
        fgets(buffer, 1023, ifp);
        // wait for write ready
        int rv = poll(&send_ufd, 1, 3500);
        if (rv == -1) {
            perror("poll write error"); // error occurred in poll()
            break;
        } else if (rv == 0) {
            printf("Timeout occurred!  Not ready for write after 3.5 seconds.\n");
        } else {
            // ready for write:
            if (send_ufd.revents & POLLOUT) {
                write(fwd_sock_fd, buffer, 1023); // write
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
        }

        rv = poll(&recv_ufd, 1, 3500);

        if (rv == -1) {
            perror("read poll errpr"); // error occurred in poll()
        } else if (rv == 0) {
            printf("Timeout occurred!  No read data after 3.5 seconds.\n");
        } else {
            // check for events on s1:
            if (recv_ufd.revents & POLLIN) {
                recv(fwd_sock_fd, buffer, 1023, 0); // receive normal data
                fprintf(ofp, "%s", buffer);
            }
            if (recv_ufd.revents & POLLPRI) {
                recv(fwd_sock_fd, buffer, 1023, MSG_OOB); // out-of-band data
                fprintf(ofp, "%s", buffer);
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
    /*while (writing || reading) {

        // write_loop
        while (1) {
            bzero(buffer, 1024);
            printf("w");
//            int read_in = read(STDIN_FILENO, buffer, 1023);
//            if (read_in < 0) {
//                error("Failed to read from stdin");
//                break;
//            }
//            printf("%d", read_in);
//            if (read_in == 0) {
//                printf("z");
//                break;
//            }
//            buffer[read_in] = '\0';
            char *inp = fgets(buffer, 1023, ifp);
            if (strlen(buffer) < 1023) {
                // probably no more input. Let's break.
                break;
            }
            n = write(fwd_sock_fd, buffer, strlen(buffer));
            if (n < 0) {
                error("ERROR writing to socket");
                break;
            }
        }
        bzero(buffer, 1024);
        //read_loop
        while (1) {
            n = recv(fwd_sock_fd, buffer, 1023, NULL);
            if (n < 0) {
                error("ERROR reading from socket");
                break;
            }
            if (n == 0) {
                break;
            } else {
                printf("%s\n", buffer);
            }
        }*/
    close(fwd_sock_fd);
    return 0;
}