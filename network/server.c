// https://www.scottklement.com/rpg/socktut/nonblocking.html#:~:text=So%2C%20to%20turn%20on%20non,tamper%20with%20the%20other%20flags)
// https://beej.us/guide/bgnet/html/split/index.html
// socket , bind it to socket, listen , accept , send/recv

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
# include <errno.h>
#define MYPORT "8080"
#define BACKLOG 2
#define MAX_CLIENTS 2
#define BUFFER_SIZE 4096
int main()
{
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    int server_socket_fd, new_fd;
    struct addrinfo hints , * res;

    int client_sockets[MAX_CLIENTS] = {0};  // Array to hold client sockets
    char buffer[BUFFER_SIZE];
    int max_sd, sd, activity, valread, i;
    fd_set readfds;

    
    memset(&hints, 0 , sizeof hints);
    hints.ai_family = AF_UNSPEC; // can be IPV4 OR IPV6
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    hints.ai_flags = AI_PASSIVE; // fill the IP automatically of the host we are currently running on.
    getaddrinfo(NULL, MYPORT, &hints, &res);

    // domain : IPV4, type : TCP , protocol : Automatic
     server_socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_socket_fd == -1) {
        perror("socket");
        return 1;
    }
    fcntl(server_socket_fd, F_SETFL, O_NONBLOCK);
    // Bind the socket to the port
    if (bind(server_socket_fd, res->ai_addr, res->ai_addrlen) == -1) {
        close(server_socket_fd);
        perror("bind");
        return 1;
    }

    // Free the address info after binding
    freeaddrinfo(res);

    // Listen for incoming connections
    if (listen(server_socket_fd, BACKLOG) == -1) {
        perror("listen");
        return 1;
    }

    printf("Server is listening on port %s\n", MYPORT);

    while (1) {

        FD_ZERO(&readfds);
        FD_SET(server_socket_fd, &readfds);
        max_sd = server_socket_fd;

        // Add client sockets to the set
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];

            // If valid socket descriptor, add to read set
            if (sd > 0)
                FD_SET(sd, &readfds);

            // Highest file descriptor number, needed for the select function
            if (sd > max_sd)
                max_sd = sd;
        }

        // Use select to wait for activity on sockets
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            printf("select error\n");
        }


        
        if (FD_ISSET(server_socket_fd, &readfds)) {
            addr_size = sizeof their_addr;
            new_fd = accept(server_socket_fd, (struct sockaddr *)&their_addr, &addr_size);
            if (new_fd < 0) {
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    perror("accept");
                }
            } else {
                printf("New connection established\n");

                // Add new socket to the client_sockets array
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (client_sockets[i] == 0) {
                        client_sockets[i] = new_fd;
                        break;
                    }
                }
            }
        }


        // Check for I/O activity on all client sockets
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) {
                // Check if the client disconnected
                valread = read(sd, buffer, BUFFER_SIZE);

                if (valread == 0) {
                    // Client has disconnected, close the socket and clear the client slot
                    printf("Client disconnected\n");
                    close(sd);
                    client_sockets[i] = 0;
                } else if (valread > 0) {
                    // Echo back the message that came in
                    buffer[valread] = '\0';
                    printf("Received: %s\n", buffer);
                    send(sd, buffer, strlen(buffer), 0);
                }
            }
        }
    }

    // Close the server socket when done
    close(server_socket_fd);
}
