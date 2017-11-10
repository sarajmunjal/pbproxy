#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

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
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    FILE *ifp = stdin;
    FILE *ofp = stdout;
    portno = args->dest_port;
    char buffer[1024];
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
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
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
        return 0;
    }
    printf("Please enter the message: ");
    bzero(buffer, 1024);
    int reading = 1;
    int writing = 1;
    while (writing || reading) {

        // write_loop
        while (1) {
            bzero(buffer, 1024);
            int read_in = read(STDIN_FILENO, buffer, 1023);
            if (read_in < 0) {
                error("Failed to read from stdin");
                break;
            }
            if (read_in == 0) {
                //
                break;
            }
            buffer[read_in] = '\0';
            n = write(sockfd, buffer, strlen(buffer));
            if (n < 0) {
                error("ERROR writing to socket");
                break;
            }
            if (n == 0) {
                // nothing to write
                continue;
            }
        }


        bzero(buffer, 1024);
        //read_loop
        while (1) {
            n = recv(sockfd, buffer, 1023, NULL);
            if (n < 0) {
                error("ERROR reading from socket");
                break;
            }
            if (n == 0) {
                break;
            } else {
                printf("%s\n", buffer);
            }
        }
    }
    close(sockfd);
    return 0;
}