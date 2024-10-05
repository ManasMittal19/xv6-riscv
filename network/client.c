
// client : socket, connect and send/recv

// https://www.scottklement.com/rpg/socktut/nonblocking.html#:~:text=So%2C%20to%20turn%20on%20non,tamper%20with%20the%20other%20flags)

// socket , bind it to socket, listen , accept , send/recv
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define MYPORT "8080"
#define MAXDATASIZE 100 // Max number of bytes we can receive at once

int main()
{
    struct addrinfo hints, *res;
    int client_socket_fd;
    int bytes_received;
    char buffer[MAXDATASIZE];

    // Set up the hints struct to specify what kind of connection we want
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     // Can use either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

    // Get the address info of the server to connect to
    getaddrinfo(NULL, MYPORT, &hints, &res);

    // Create a socket
    client_socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    // Connect to the server
    connect(client_socket_fd, res->ai_addr, res->ai_addrlen);

    // Send a message to the server
    char *msg = "Hello from the client!";
    int len = strlen(msg);
    int bytes_sent = send(client_socket_fd, msg, len, 0);
    if (bytes_sent == -1) {
        perror("send");
        exit(1);
    }

    // Receive data from the server
    bytes_received = recv(client_socket_fd, buffer, MAXDATASIZE - 1, 0);
    if (bytes_received == -1) {
        perror("recv");
        exit(1);
    }
    buffer[bytes_received] = '\0'; // Null-terminate the received string

    printf("Client received: '%s'\n", buffer);

    // Clean up
    close(client_socket_fd);
    freeaddrinfo(res);

    return 0;
}
