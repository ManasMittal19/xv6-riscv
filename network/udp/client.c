#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT "16348" // port on which the server is running
#define MAXDATASIZE 1000 // Max number of bytes we can receive at once

int main(int argc, char * argv[])
{
    struct addrinfo hints, *res;
    int client_socket_fd;
    int bytes_received;
    char buffer[MAXDATASIZE];
    char *server_ip;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        exit(1);
    }
    server_ip = argv[1];

    printf("Attempting to connect to server at IP: %s, Port: %s\n", server_ip, PORT);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    int status = getaddrinfo(server_ip, PORT, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    client_socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (client_socket_fd == -1) {
        perror("client: socket");
        exit(1);
    }

   // Rather than connecting to server in TCP , here we are sending a CONNECT message to the server
    // A way to tell the server about our presence 
    const char *initial_message = "CONNECT";
    int bytes_sent = sendto(client_socket_fd, initial_message, strlen(initial_message), 0, res->ai_addr, res->ai_addrlen);
    if (bytes_sent == -1) {
        perror("sendto");
        exit(1);
    }

    // printf("Sent initial connection message to server\n");

    while (1)
    {
        bytes_received = recvfrom(client_socket_fd, buffer, MAXDATASIZE - 1, 0, NULL, NULL);
        if (bytes_received == -1) {
            perror("recvfrom");
            exit(1);
        }
        
        buffer[bytes_received] = '\0'; // Null-terminate the received string
        printf("%s", buffer);

        if (strstr(buffer, "Enter your move") != NULL)
        {
            char move[50];
            if (fgets(move, sizeof(move), stdin) == NULL) {
                perror("fgets");
                break;
            }
            int len = strlen(move);
            int bytes_sent = sendto(client_socket_fd, move, len, 0, res->ai_addr, res->ai_addrlen);
            if (bytes_sent == -1) {
                perror("sendto");
                exit(1);
            }
        }
        else if (strstr(buffer, "Would you like to play again? (yes/no): ") != NULL)
        {
            char response[10];
            if (fgets(response, sizeof(response), stdin) == NULL) {
                perror("fgets");
                break;
            }
            int len = strlen(response);
            int bytes_sent = sendto(client_socket_fd, response, len, 0, res->ai_addr, res->ai_addrlen);
            if (bytes_sent == -1) {
                perror("sendto");
                exit(1);
            }
            
            if (strncmp(response, "no", 2) == 0) {
                printf("Thanks for playing! Goodbye.\n");
                break;
            }
        }

        memset(buffer, 0, MAXDATASIZE);
    }

    // Clean up
    close(client_socket_fd);
    freeaddrinfo(res);

    return 0;
}
