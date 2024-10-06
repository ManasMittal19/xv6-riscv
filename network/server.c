// https://www.scottklement.com/rpg/socktut/nonblocking.html#:~:text=So%2C%20to%20turn%20on%20non,tamper%20with%20the%20other%20flags)
// https://beej.us/guide/bgnet/html/split/index.html
// socket , bind it to socket, listen , accept , send/recv + Non blocking sockets for mulitple clients


// including all the networking and related libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>
#include <ctype.h>

int players = 0;
int turn = 0;

#define TURN_MESSAGE "TURN\n"

#define MYPORT "16344" // The port the we are currently running on 
#define BACKLOG 2 // the maximum number of pending clients in queue 
#define MAX_CLIENTS 2 // the maximum number of clients 
#define BUFFER_SIZE 65535 // buffer size of sending and receiving data


// Some game declarations : 
# define SIZE 3 
# define EMPTY_CELL ' '
# define X 'X'
# define O 'O'

char board[SIZE][SIZE];
int game_ended = 0; 

// initialize the board with empty cells
void initialize_board() {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            board[i][j] = EMPTY_CELL;
        }
    }
}


void broadcast_message(int client_sockets[], const char* message) {
    for (int j = 0; j < MAX_CLIENTS; j++) {
        if (client_sockets[j] != 0) {
            send(client_sockets[j], message, strlen(message), 0);
        }
    }
}

char* board_to_string() {
    static char board_str[SIZE * (SIZE * 4 + 3) + (SIZE - 1) * 12 + 1];  // Increased size to accommodate lines
    int index = 0;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            board_str[index++] = ' ';
            board_str[index++] = board[i][j];
            board_str[index++] = ' ';
            if (j < SIZE - 1) {
                board_str[index++] = '|';
            }
        }
        board_str[index++] = '\n';
        if (i < SIZE - 1) {
            strcpy(&board_str[index], "---+---+---\n");
            index += 12;
        }
    }
    board_str[index] = '\0';
    return board_str;
}

void send_board_to_client(int client_socket)
{
    char* board_str = board_to_string();
    char message[BUFFER_SIZE];
    snprintf(message, BUFFER_SIZE, "Current board state:\n%s", board_str);
    send(client_socket, message, strlen(message), 0);
}

void update_board(int row, int col, char symbol) {
    board[row][col] = symbol;
}

// for a given symbol check if there is a winning pattern or not
int check_winner(char symbol) {

    for (int i = 0; i < SIZE; i++) {
        if (board[i][0] == symbol && board[i][1] == symbol && board[i][2] == symbol) {
            return 1;   
        }
        if (board[0][i] == symbol && board[1][i] == symbol && board[2][i] == symbol) {
            return 1;
        }
    }
    if (board[0][0] == symbol && board[1][1] == symbol && board[2][2] == symbol) {
        return 1;
    }
    if (board[0][2] == symbol && board[1][1] == symbol && board[2][0] == symbol) {
        return 1;
    }
    return 0;
}

int no_moves_left() {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == EMPTY_CELL) {
                return 0;
            }
        }
    }
    return 1;
}   
// if we can put the symbol there or not
int isValid(int row, int col)
{
    if(row < 0 || row > 2  || col < 0 || col > 2)
    return 0;
    if(board[row][col] == ' ')
    {
        return 1;
    }
    return 0;
}

#define PLAY_AGAIN_MESSAGE "Would you like to play again? (yes/no): "
#define YES_RESPONSE "yes"
#define NO_RESPONSE "no"

int handle_play_again(int client_sockets[])
{
    char responses[MAX_CLIENTS][BUFFER_SIZE];
    int play_again_count = 0;
    int responses_received = 0;

    // Initialize responses array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        responses[i][0] = '\0';
    }

    // Ask both players if they want to play again
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != 0) {
            send(client_sockets[i], PLAY_AGAIN_MESSAGE, strlen(PLAY_AGAIN_MESSAGE), 0);
        }
    }

    // Wait for responses from both players
    while (responses_received < MAX_CLIENTS) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] != 0 && responses[i][0] == '\0') {
                int valread = recv(client_sockets[i], responses[i], BUFFER_SIZE, 0);
                if (valread > 0) {
                    responses[i][valread] = '\0';
                    // Remove newline characters
                    responses[i][strcspn(responses[i], "\n")] = 0;
                    printf("Player %d response: %s\n", i + 1, responses[i]);
                    
                    // Convert response to lowercase for case-insensitive comparison
                    for (int j = 0; responses[i][j]; j++) {
                        responses[i][j] = tolower(responses[i][j]);
                    }

                    if (strncmp(responses[i], YES_RESPONSE, strlen(YES_RESPONSE)) == 0) {
                        printf("Player %d wants to play again\n", i + 1);
                        play_again_count++;
                    }
                    responses_received++;
                }
            }
        }
    }

    // Handle different scenarios
    if (play_again_count == MAX_CLIENTS) {
        // Both want to play again
        broadcast_message(client_sockets, "Both players agreed to play again. Starting a new game.\n");
        
        return 1;
    } else if (play_again_count == 0) {
        // Both don't want to play again
        broadcast_message(client_sockets, "Both players chose not to play again. Ending the game.\n");
    } else {
        // One wants to play, the other doesn't
        broadcast_message(client_sockets, "One player wants to play again, but the other doesn't. Ending the game.\n");
    }
    initialize_board();
    turn = 0;

    return 0;
}

int main()
{
    struct sockaddr_storage their_addr; //  Used to store client address once the connection is make
    socklen_t addr_size; // size of the client address
    int server_socket_fd; // file descriptor for server 
    int new_fd; // just a variable 
    struct addrinfo hints , * res; // hints is passed to getaddrinfo() and in *res we get the output

    int client_sockets[MAX_CLIENTS] = {0};  // Array to hold client sockets
    char buffer[BUFFER_SIZE]; // our buffer
    int max_sd, sd, activity, valread, i; // variables to handle multiple clients using teh select function 
    fd_set readfds;

    
    // setting up hints before passing it as parameter
    memset(&hints, 0 , sizeof hints);
    hints.ai_family = AF_UNSPEC; // can be IPV4 OR IPV6
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    hints.ai_flags = AI_PASSIVE; // fill the IP automatically of the host we are currently running on and bind to all interfaces 
    getaddrinfo(NULL, MYPORT, &hints, &res); // IP address : NULL (will be assigned automatically), Port number : MYPORT, hints is passed and res is received
    // res has important this like ai_family , sock_type, protocol etc.

   // all of the paramter that are needed to be passed are already in the res.

    server_socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_socket_fd == -1) {
        // error handling for socket()
        perror("socket");
        return 1;
    }

    // by default accept() and recv() are blocking , we can changing it.
    fcntl(server_socket_fd, F_SETFL, O_NONBLOCK);


    // Bind the socket to the port
    if (bind(server_socket_fd, res->ai_addr, res->ai_addrlen) == -1) {
        // error handling for bind()
        close(server_socket_fd);
        perror("bind");
        return 1;
    }

    // Free the address info after binding , as we no longer need it
    freeaddrinfo(res);


    // Listen for incoming connections
    if (listen(server_socket_fd, BACKLOG) == -1) {
        perror("listen");
        return 1;
    }

    printf("Server is listening on port %s\n", MYPORT);
    initialize_board();
  
    while (1) {
        // readfds : It is a file descriptor set that holds the file descriptors that the server want to monitor for "read" events.
        FD_ZERO(&readfds); 
        FD_SET(server_socket_fd, &readfds); // add the server socket to the readfds
        max_sd = server_socket_fd; // intialization:  set the max_sd to the server socket

        // Add client sockets to the readfds set
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i]; 

            // If valid socket descriptor, add to read set
            if (sd > 0)
                FD_SET(sd, &readfds);

            // Highest file descriptor number, needed for the select function
            if (sd > max_sd)
                max_sd = sd;
        }

        // select basically looks for at all the file descriptors in the readfds set and looks for those who are ready for I/O activity.
        // max_sd + 1 helps in telling the upper bound to select to check upto
        // select just retain those file desriptor which are ready for read and remove other. Thus enabling us to use FD_ISSET later to check.
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        // writefds, exceptfds and timeout are marked NULL.

        // activity cannot be zero as our timeout is NULL so it will wait atleast one of the clients get ready for some I/O.
        //
        if ((activity < 0) && (errno != EINTR)) {
            printf("select error\n");
        }
        // FD_ISSET check if the server socket is ready to read
        if (FD_ISSET(server_socket_fd, &readfds)) 
        {
            addr_size = sizeof their_addr;
            new_fd = accept(server_socket_fd, (struct sockaddr *)&their_addr, &addr_size); // accepting the connectoin
            if (new_fd < 0) {
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    // error handling for accept : a little bit different from older approach where accept() used to block
                    perror("accept");
                }
            } else 
            {
                printf("New connection established\n");

                // Add new socket to the client_sockets array
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (client_sockets[i] == 0) {
                        client_sockets[i] = new_fd;
                        players++;
                        if (i == 0)
                        {
                            printf("Player X connected....\n");
                        }
                        else if(i == 1)
                        {
                            printf("Player O connected.....\n");
                        }
                        if(players == MAX_CLIENTS)
                        {
                            printf("Both player connected. Game starts now.\n");
                            char start_message[BUFFER_SIZE];
                            char* board_str = board_to_string();
                            
                            for (int j = 0; j < MAX_CLIENTS; j++) {
                                if (client_sockets[j] != 0) {
                                    char player_symbol = (j == 0) ? 'X' : 'O';
                                    snprintf(start_message, BUFFER_SIZE, 
                                        "Game has started. You are Player %c.\n"
                                        "Player X goes first.\n\n"
                                        "Current board state:\n%s", 
                                        player_symbol, board_str);
                                    send(client_sockets[j], start_message, strlen(start_message), 0);
                                }
                            }
                            if (send(client_sockets[0], TURN_MESSAGE, strlen(TURN_MESSAGE), 0) == -1) {
                                    perror("send turn message");
                            }
                        }
                        break;
                    }
                }
            }
        }

        for (i = 0; i < MAX_CLIENTS; i++) 
        {
            sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) 
            {
                // Check if the client disconnected
                valread = read(sd, buffer, BUFFER_SIZE);

                if (valread == 0) {
                    // Client has disconnected, close the socket and clear the client slot
                    printf("Player %d disconnected\n", i + 1);
                    close(sd);
                    client_sockets[i] = 0;
                    game_ended = 1; // one of the players disconnected
                } else if (valread > 0) 
                {
                    // Main logic for the game
                    if(players == MAX_CLIENTS)
                    {
                        buffer[valread] = '\0';
                        int row, col;
                        if(sscanf(buffer, "%d %d", &row, &col) == 2)
                        {
                            row--;
                            col--;
                            if(isValid(row, col))
                            {
                                if(turn == 0)
                                update_board(row, col , X);
                                else update_board(row, col , O);
                                
                               // checking if someone won or not
                                if(turn == 0)
                                {
                                    if(check_winner(X))
                                    {
                                        game_ended = 1;
                                        char win_message[BUFFER_SIZE];
                                        snprintf(win_message, BUFFER_SIZE, "Player 1 Wins!\n%s", board_to_string());
                                        broadcast_message(client_sockets, win_message);
                                    }
                                }
                                else 
                                {
                                    if(check_winner(O))
                                    {
                                        game_ended = 1;
                                        char win_message[BUFFER_SIZE];
                                        snprintf(win_message, BUFFER_SIZE, "Player 2 Wins!\n%s", board_to_string());
                                        broadcast_message(client_sockets, win_message);
                                    }
                                }
                                if(game_ended == 0)
                                {
                                    // check for draw
                                    if(no_moves_left() == 1) // no move left
                                    {
                                        game_ended = 1;
                                         char draw_message[BUFFER_SIZE];
                                        snprintf(draw_message, BUFFER_SIZE, "It's a draw!\n%s", board_to_string());
                                        broadcast_message(client_sockets, draw_message);
                                    }
                                }
                                if(game_ended == 0)
                                {
                                    for (int j = 0; j < MAX_CLIENTS; j++) {
                                    if (client_sockets[j] != 0) {
                                            send_board_to_client(client_sockets[j]);
                                        }
                                    }
                                     turn = 1 - turn; // changing the turns
                                    if (send(client_sockets[turn], TURN_MESSAGE, strlen(TURN_MESSAGE), 0) == -1) {
                                        perror("send turn message");
                                    }
                                }
                                
                                if (game_ended)
                                {
                                    if(handle_play_again(client_sockets))
                                    {
                                        game_ended = 0;
                                        turn = 0;
                                        char* initial_board = board_to_string();
                                        char start_message[BUFFER_SIZE];
                                        snprintf(start_message, BUFFER_SIZE, 
                                            "New game started. Player X goes first.\n\n"
                                            "Current board state:\n%s", 
                                            initial_board);
                                        broadcast_message(client_sockets, start_message);
                                    
                                        if (send(client_sockets[0], TURN_MESSAGE, strlen(TURN_MESSAGE), 0) == -1) {
                                            perror("send turn message");
                                        }
                                    }
                                    else
                                    {
                                        printf("Resetting\n");
                                        for (int j = 0; j < MAX_CLIENTS; j++)
                                        {
                                            if (client_sockets[j] != 0)
                                            {
                                                close(client_sockets[j]);
                                                FD_CLR(client_sockets[j], &readfds);
                                                client_sockets[j] = 0;
                                            }
                                        }
                                        players = 0;
                                        break;
                                    }
                                }
                                
                            }
                            else 
                            {
                                char message[BUFFER_SIZE];
                                snprintf(message, BUFFER_SIZE, "Enter valid position\n");
                                send(client_sockets[turn], message, strlen(message), 0);
                                if (send(client_sockets[turn], TURN_MESSAGE, strlen(TURN_MESSAGE), 0) == -1) {
                                    perror("send turn message");
                                }
                            }
                        }
                    }
                }
            }
        }

    // Close the server socket when done
    }
    close(server_socket_fd);
}
