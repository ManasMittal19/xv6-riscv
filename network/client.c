// client : socket, connect and send/recv

// https://www.scottklement.com/rpg/socktut/nonblocking.html#:~:text=So%2C%20to%20turn%20on%20non,tamper%20with%20the%20other%20flags)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define PORT "16344" // port on which the server is running
#define MAXDATASIZE 1000 // Max number of bytes we can receive at once

int main()
{
    struct addrinfo hints, *res;
    int client_socket_fd;
    int bytes_received;
    char buffer[MAXDATASIZE];

    memset(&hints, 0, sizeof hints); // intialization to 0
    hints.ai_family = AF_UNSPEC;     // Can use either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

    getaddrinfo("127.0.0.1", PORT, &hints, &res); // Get the address info of the server to connect to

    client_socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    // Connect to the server
    if (connect(client_socket_fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        exit(1);
    }

    // printf("Client started. Waiting for data...\n");

    while (1)
    {
        // printf("Waiting to receive data...\n");
        bytes_received = recv(client_socket_fd, buffer, MAXDATASIZE - 1, 0);
        // printf("Received %d bytes\n", bytes_received);

        if (bytes_received == -1) {
            perror("recv");
            exit(1);
        } else if (bytes_received == 0) {
            printf("Server closed the connection\n");
            break;
        }
        
        buffer[bytes_received] = '\0'; // Null-terminate the received string
        // printf("Received buffer: '%s'\n", buffer);

        if (strstr(buffer, "TURN") != NULL)
        {
            printf("%s", buffer);
            // printf("TURN detected. Your move.\n");
            printf("Enter your move : (row , col)\n");
            char str[50];
            scanf(" %[^\n]", str);
            int len = strlen(str);
            int bytes_sent = send(client_socket_fd, str , len, 0);
            if (bytes_sent == -1) {
                perror("send");
                exit(1);
            }
            // printf("Sent %d bytes\n", bytes_sent);
        }
        else if (strstr(buffer, "Would you like to play again? (yes/no): ") != NULL)
        {
            printf("%s", buffer);
            char response[10];
            scanf(" %[^\n]", response);
            int len = strlen(response);
            int bytes_sent = send(client_socket_fd, response, len, 0);
            if (bytes_sent == -1) {
                perror("send");
                exit(1);
            }
            
            if (strcmp(response, "no") == 0) {
                printf("Thanks for playing! Goodbye.\n");
                break;
            }
        }
        else {
            // printf("Displaying received message:\n");
            printf("%s", buffer);
        }
       
        // Clear the buffer after processing
        memset(buffer, 0, MAXDATASIZE);
        // printf("Buffer cleared. Loop ending.\n");
    }

    // Clean up
    close(client_socket_fd);
    freeaddrinfo(res);

    return 0;
}
