#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BACKLOG 10         // How many pending connections queue will hold
#define BUFFER_SIZE 1024   // Buffer size for receiving data

/* SIGCHLD handler to reap dead child processes */
void sigchld_handler(int s) {
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

/* get_in_addr: Returns pointer to the address (IPv4 or IPv6) */
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* Helper function to reverse a string in place */
void reverse_string(char *str) {
    int len = strlen(str);
    int i = 0, j = len - 1;
    while (i < j) {
        char tmp = str[i];
        str[i] = str[j];
        str[j] = tmp;
        i++;
        j--;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    char *port = argv[1];
    int sockfd, new_fd;  // sockfd: listening socket, new_fd: new connection socket
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];

    /* Set up the hints structure */
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;     // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;   // TCP stream sockets
    hints.ai_flags    = AI_PASSIVE;     // Use my IP

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(EXIT_FAILURE);
    }
    
    /* Loop through all the results and bind to the first we can */
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }
        
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        
        break;
    }
    
    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(EXIT_FAILURE);
    }
    
    freeaddrinfo(servinfo); // All done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    /* Set up the SIGCHLD handler to reap dead child processes */
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    
    printf("Server successfully started listening on port %s\n", port);

    /* Main accept() loop */
    while (1) {
        struct sockaddr_storage their_addr;
        socklen_t sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("Server: got connection from %s\n", s);
        
        if (!fork()) {  // This is the child process
            close(sockfd);  // Child doesn't need the listener
            
            /* Receive data from the client */
            char buffer[BUFFER_SIZE];
            memset(buffer, 0, sizeof(buffer));
            int numbytes = recv(new_fd, buffer, sizeof(buffer) - 1, 0);
            if (numbytes == -1) {
                perror("recv");
                close(new_fd);
                exit(EXIT_FAILURE);
            }
            buffer[numbytes] = '\0';  // Ensure null termination
            printf("Received: %s\n", buffer);
            
            /* Process the command: expecting "INVERT <word>" */
            char command[BUFFER_SIZE], word[BUFFER_SIZE];
            if (sscanf(buffer, "%s %s", command, word) == 2) {
                if (strcmp(command, "INVERT") == 0) {
                    /* Reverse the word */
                    char reversed[BUFFER_SIZE];
                    strncpy(reversed, word, BUFFER_SIZE - 1);
                    reversed[BUFFER_SIZE - 1] = '\0';
                    reverse_string(reversed);
                    int len = strlen(reversed);
                    
                    /* Build the response */
                    char response[BUFFER_SIZE];
                    snprintf(response, sizeof(response),
                             "200 OK\nContent-Length: %d\nInversion: %s\n",
                             len, reversed);
                    
                    printf("Sent: 200 OK\nSent: Content-Length: %d\nSent: Inversion: %s\n", len, reversed);
                    if (send(new_fd, response, strlen(response), 0) == -1) {
                        perror("send");
                    }
                } else {
                    /* Command not recognized */
                    char *errMsg = "501 NOT IMPLEMENTED\n";
                    send(new_fd, errMsg, strlen(errMsg), 0);
                    printf("Sent: 501 NOT IMPLEMENTED\n");
                }
            } else {
                /* Invalid command format */
                char *errMsg = "501 NOT IMPLEMENTED\n";
                send(new_fd, errMsg, strlen(errMsg), 0);
                printf("Sent: 501 NOT IMPLEMENTED\n");
            }
            
            close(new_fd);
            printf("Client connection finished!\n");
            exit(EXIT_SUCCESS);
        }
        close(new_fd);  // Parent doesn't need this socket descriptor
    }
    
    return 0;
}
