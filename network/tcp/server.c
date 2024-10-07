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

#define TURN_MESSAGE "TURN\n"
#define PLAY_AGAIN_MESSAGE "Would you like to play again? (yes/no): "
#define YES_RESPONSE "yes"
#define NO_RESPONSE "no"

#define MYPORT "16347" // The port the we are currently running on 
#define BACKLOG 2 // the maximum number of pending clients in queue 
#define MAX_CLIENTS 2 // the maximum number of clients 
#define BUFFER_SIZE 65535 // buffer size of sending and receiving data


// Some game declarations : 
# define SIZE 3 
# define EMPTY_CELL ' '
# define X 'X'
# define O 'O'

// Game state
char board[SIZE][SIZE];
int game_ended = 0;
int turn = 0;
int players = 0;

// Function prototypes
int setup_server();
void handle_new_connection(int server_socket_fd, int client_sockets[]);
void handle_client_activity(int client_sockets[], fd_set *readfds);
void broadcast_message(int client_sockets[], const char* message);
void send_board_to_client(int client_socket);
void initialize_board();
char* board_to_string();
void update_board(int row, int col, char symbol);
int check_winner(char symbol);
int no_moves_left();
int isValid(int row, int col);
int handle_play_again(int client_sockets[]);

int main() {
    int server_socket_fd = setup_server();
    if (server_socket_fd == -1) {
        return 1;
    }

    int client_sockets[MAX_CLIENTS] = {0};
    fd_set readfds; // a set of file descriptor holding all the sockets ready for some read.

    initialize_board(); 
    printf("Server is listening on port %s\n", MYPORT);

    while (1) {
        // filling up the readfds with all the fd which are available for read

        FD_ZERO(&readfds);
        FD_SET(server_socket_fd, &readfds);
        int max_sd = server_socket_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }
        // max_sd upperbound the select function
        // no write_fds, except_fds and timeout
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        // activity cannot be zero as there is not timeout
        // if activity positive everything is correct otherwise error handling
        if ((activity < 0) && (errno != EINTR)) {
            printf("select error\n");
            continue;
        }

        if (FD_ISSET(server_socket_fd, &readfds)) {
            // inviting the new player who just joined the game
            handle_new_connection(server_socket_fd, client_sockets);
        }
        // actual logic of playing the game
        handle_client_activity(client_sockets, &readfds);
    }

    close(server_socket_fd);
    return 0;
}

int setup_server() {
    // contains all the code for setting up a server with non blocking sockets
    struct addrinfo hints, *res;
    int server_socket_fd;
    // hints is passed as argument to getaddrinfo and information is received back in res.
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // either IPV4 or IPV6
    hints.ai_socktype = SOCK_STREAM; //TCP
    hints.ai_flags = AI_PASSIVE; // to accept incoming connections.

    getaddrinfo(NULL, MYPORT, &hints, &res); 
    // NULL : do not pass any specific IP
    // MYPORT : the port number 
    // hints is passed 
    // res is passed to get the result that we use later


    // creating the socket
    server_socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_socket_fd == -1) {
        perror("socket");
        return -1;
    }

    // changing the socket file descriptor as non blockings
    fcntl(server_socket_fd, F_SETFL, O_NONBLOCK);

    // bind
    if (bind(server_socket_fd, res->ai_addr, res->ai_addrlen) == -1) {
        close(server_socket_fd);
        perror("bind");
        return -1;
    }
    // removing the res as it is no longer needed
    freeaddrinfo(res);

    // listening to a socket.
    // BACKLOG : maximum number of clients that can be in the queue
    if (listen(server_socket_fd, BACKLOG) == -1) {
        perror("listen");
        return -1;
    }

    return server_socket_fd;
}

void handle_new_connection(int server_socket_fd, int client_sockets[]) {

    // acceptnig the new requrest
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof their_addr;
    int new_fd = accept(server_socket_fd, (struct sockaddr *)&their_addr, &addr_size);

    if (new_fd < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            perror("accept");
        }
        return;
    }

    printf("New connection established\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == 0) {
            client_sockets[i] = new_fd;
            players++; // incrementing the number of player we have
            // assinging the first player X and second player O
            printf("Player %c connected....\n", (i == 0) ? 'X' : 'O');

            if (players == MAX_CLIENTS) {
                // starting game when both player connected
                printf("Both players connected. Game starts now.\n");
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
                            // sending the start message to the players
                        send(client_sockets[j], start_message, strlen(start_message), 0);
                    }
                }
                // TURN_MESSAGE : is a special message send to the person having the next turn
                // since the game has just started we are sending it to just the player 1 (indexed as 0)
                if (send(client_sockets[0], TURN_MESSAGE, strlen(TURN_MESSAGE), 0) == -1) {
                    perror("send turn message");
                }
            }
            break;
        }
    }
}

void handle_client_activity(int client_sockets[], fd_set *readfds) {
    char buffer[BUFFER_SIZE];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        int sd = client_sockets[i];

        if (FD_ISSET(sd, readfds)) {
            int valread = read(sd, buffer, BUFFER_SIZE);

            if (valread == 0) {
                printf("Player %d disconnected\n", i + 1);
                close(sd);
                client_sockets[i] = 0;
                players--;
                if (players > 0) {
                    // if there is some other player  , tell him appropriate message
                    char message[] = "Other player disconnected. Waiting for new player.\n";
                    send(client_sockets[1-i], message, strlen(message), 0);
                }
                // reintializing the board and turn 
                initialize_board();
                turn = 0;
                break;
            } else if (valread > 0 && players == MAX_CLIENTS) {
                buffer[valread] = '\0';
                int row, col;
                if (sscanf(buffer, "%d %d", &row, &col) == 2) {
                    row--; col--;
                    // check for valid move  : inside grid and empty sell are allowed
                    if (isValid(row, col)) {
                        // depending on the turn make the board
                        update_board(row, col, (turn == 0) ? X : O);
                        
                        // checking for winner : this function has all the winner pattern
                        if (check_winner((turn == 0) ? X : O)) {
                            game_ended = 1;
                            char win_message[BUFFER_SIZE];
                            snprintf(win_message, BUFFER_SIZE, "\033[0;34mPlayer %d Wins!\033[0m\n%s", turn + 1, board_to_string());
                            broadcast_message(client_sockets, win_message);
                        } else if (no_moves_left()) {
                            // if the entire grid is full : print a draw message
                            game_ended = 1;
                            char draw_message[BUFFER_SIZE];
                            snprintf(draw_message, BUFFER_SIZE, "\033[0mIt's a draw!\n\033[0m%s", board_to_string());
                            broadcast_message(client_sockets, draw_message);
                        }

                        if (!game_ended) {
                            // if game doesn't end : send board to all clients
                            for (int j = 0; j < MAX_CLIENTS; j++) {
                                if (client_sockets[j] != 0) {
                                    send_board_to_client(client_sockets[j]);
                                }
                            }
                            turn = 1 - turn; // change the turn 
                            // and send the turn message
                            if (send(client_sockets[turn], TURN_MESSAGE, strlen(TURN_MESSAGE), 0) == -1) {
                                perror("send turn message");
                            }
                        } else 
                        {
                            // game ended. Ask the users if they would play again
                            if (handle_play_again(client_sockets)) {
                                // when both of them agrees
                                initialize_board();
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
                               
                            } else {
                                // Close connections if players don't want to play again
                                for (int j = 0; j < MAX_CLIENTS; j++) {
                                    if (client_sockets[j] != 0) {
                                        close(client_sockets[j]);
                                        client_sockets[j] = 0;
                                    }
                                }
                                players = 0;
                                initialize_board();
                                turn =0;
                                break;
                            }
                             game_ended = 0;
                        }
                    } else {
                        // invalid move
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

void broadcast_message(int client_sockets[], const char* message) {
    for (int j = 0; j < MAX_CLIENTS; j++) {
        if (client_sockets[j] != 0) {
            send(client_sockets[j], message, strlen(message), 0);
        }
    }
}

void send_board_to_client(int client_socket) {
    char* board_str = board_to_string();
    char message[BUFFER_SIZE];
    snprintf(message, BUFFER_SIZE, "Current board state:\n%s", board_str);
    send(client_socket, message, strlen(message), 0);
}

void initialize_board() {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            board[i][j] = EMPTY_CELL;
        }
    }
    game_ended = 0;
}

char* board_to_string() {
    static char board_str[SIZE * (SIZE * 4 + 3) + (SIZE - 1) * 12 + 1];
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

void update_board(int row, int col, char symbol) {
    board[row][col] = symbol;
}

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

int isValid(int row, int col) {
    if (row < 0 || row > 2 || col < 0 || col > 2) {
        return 0;
    }
    if (board[row][col] == EMPTY_CELL) {
        return 1;
    }
    return 0;
}

int handle_play_again(int client_sockets[]) {
    char responses[MAX_CLIENTS][BUFFER_SIZE];
    int play_again_count = 0;
    int responses_received = 0;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        responses[i][0] = '\0';
    }

    broadcast_message(client_sockets, PLAY_AGAIN_MESSAGE);

    while (responses_received < MAX_CLIENTS) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] != 0 && responses[i][0] == '\0') {
                int valread = recv(client_sockets[i], responses[i], BUFFER_SIZE, 0);
                if (valread > 0) {
                    responses[i][valread] = '\0';
                    responses[i][strcspn(responses[i], "\n")] = 0;
                    printf("Player %d response: %s\n", i + 1, responses[i]);
                    
                    for (int j = 0; responses[i][j]; j++) {
                        responses[i][j] = tolower(responses[i][j]);
                    }

                    if (strncmp(responses[i], YES_RESPONSE, strlen(YES_RESPONSE)) == 0) {
                        play_again_count++;
                    }
                    responses_received++;
                }
            }
        }
    }

    // considering all possible response cases from the clients

    if (play_again_count == MAX_CLIENTS) {
        broadcast_message(client_sockets, "Both players agreed to play again. Starting a new game.\n");
        return 1;
    } else if (play_again_count == 0) {
        broadcast_message(client_sockets, "Both players chose not to play again. Ending the game.\n");
    } else {
        broadcast_message(client_sockets, "One player wants to play again, but the other doesn't. Ending the game.\n");
    }
    return 0;
}