#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAXDATASIZE 100  

// get_in_addr: returns pointer to the address (IPv4 or IPv6)
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server IP> <server port> <word>\n", argv[0]);
        exit(1);
    }

    // Set up hints for getaddrinfo
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;      // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;  // TCP stream sockets

    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // Loop through all results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    printf("client: connecting to %s\n", s);
    freeaddrinfo(servinfo);  // Done with this structure

    // Build the command "INVERT <word>"
    char command[256];
    snprintf(command, sizeof(command), "INVERT %s", argv[3]);

    // Send the command to the server
    int len = strlen(command);
    if (send(sockfd, command, len, 0) == -1) {
        perror("send");
        exit(1);
    }

    // Receive the server's response and display it
    printf("client: received response:\n");
    while ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) > 0) {
        buf[numbytes] = '\0';
        printf("%s", buf);
    }
    if (numbytes < 0) {
        perror("recv");
    }
    printf("Client exiting successfully!\n");
    close(sockfd);
    return 0;
}
