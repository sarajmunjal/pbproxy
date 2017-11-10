#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <unistd.h>

void error(char *str) {
    perror(str);
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    FILE *ifp = stdin;
    FILE *ofp = stdout;
    portno = 12345;
    char buffer[1024];
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    char *hostname = "localhost";
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