// UDP server

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
#include <arpa/inet.h>

#define MYPORT "16348"
#define MAX_CLIENTS 2
#define BUFFER_SIZE 1024
#define BOARD_SIZE 3

#define TURN_MESSAGE "Enter your move (row col): "
#define PLAY_AGAIN_MESSAGE "Would you like to play again? (yes/no): "
#define YES_RESPONSE "yes"
#define NO_RESPONSE "no"

struct client_info {
    struct sockaddr_storage addr;
    socklen_t addr_len;
    char symbol;  // 'X' or 'O'
};

struct client_info clients[MAX_CLIENTS];
int num_clients = 0;

char board[BOARD_SIZE][BOARD_SIZE];
int game_ended = 0;
int turn = 0;

// Add these new function prototypes
void reset_game();
int make_move(int row, int col, char symbol);
int check_winner(char symbol);
int is_board_full();
void handle_play_again(int server_socket_fd, int client_index, const char* response);

int setup_server() {
    struct addrinfo hints, *res;
    int server_socket_fd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, MYPORT, &hints, &res);

    server_socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_socket_fd == -1) {
        perror("socket");
        return -1;
    }

    int flags = fcntl(server_socket_fd, F_GETFL, 0);
    fcntl(server_socket_fd, F_SETFL, flags | O_NONBLOCK);

    if (bind(server_socket_fd, res->ai_addr, res->ai_addrlen) == -1) {
        close(server_socket_fd);
        perror("bind");
        return -1;
    }

    freeaddrinfo(res);
    return server_socket_fd;
}

int find_client(struct sockaddr_storage *addr, socklen_t addr_len) {
    for (int i = 0; i < num_clients; i++) {
        if (memcmp(&clients[i].addr, addr, addr_len) == 0) {
            return i;
        }
    }
    return -1;
}

int add_new_client(struct sockaddr_storage *addr, socklen_t addr_len) {
    if (num_clients >= MAX_CLIENTS) {
        return -1;
    }
    memcpy(&clients[num_clients].addr, addr, addr_len);
    clients[num_clients].addr_len = addr_len;
    clients[num_clients].symbol = (num_clients == 0) ? 'X' : 'O';
    return num_clients++;
}

void send_to_client(int server_socket_fd, int client_index, const char *message) {
    sendto(server_socket_fd, message, strlen(message), 0,
           (struct sockaddr *)&clients[client_index].addr, clients[client_index].addr_len);
}

void broadcast_message(int server_socket_fd, const char *message) {
    for (int i = 0; i < num_clients; i++) {
        send_to_client(server_socket_fd, i, message);
    }
}

void initialize_board() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = ' ';
        }
    }
    game_ended = 0;
    turn = 0;
}

char* board_to_string() {
    static char board_str[100];
    snprintf(board_str, sizeof(board_str),
             " %c | %c | %c \n---+---+---\n %c | %c | %c \n---+---+---\n %c | %c | %c \n",
             board[0][0], board[0][1], board[0][2],
             board[1][0], board[1][1], board[1][2],
             board[2][0], board[2][1], board[2][2]);
    return board_str;
}

int check_winner(char symbol) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i][0] == symbol && board[i][1] == symbol && board[i][2] == symbol) return 1;
        if (board[0][i] == symbol && board[1][i] == symbol && board[2][i] == symbol) return 1;
    }
    if (board[0][0] == symbol && board[1][1] == symbol && board[2][2] == symbol) return 1;
    if (board[0][2] == symbol && board[1][1] == symbol && board[2][0] == symbol) return 1;
    return 0;
}

int is_board_full() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == ' ') return 0;
        }
    }
    return 1;
}

void handle_new_connection(int server_socket_fd, int client_index) {
    char message[BUFFER_SIZE];
    snprintf(message, BUFFER_SIZE, "Welcome! You are player %c. Waiting for another player to join...\n", clients[client_index].symbol);
    send_to_client(server_socket_fd, client_index, message);

    if (num_clients == MAX_CLIENTS) {
        initialize_board();
        broadcast_message(server_socket_fd, "Both players have joined. Game starts now!\n");
        broadcast_message(server_socket_fd, board_to_string());
        send_to_client(server_socket_fd, 0, TURN_MESSAGE);
    }
}

void reset_game() {
    initialize_board();
    game_ended = 0;
    turn = 0;
}

int make_move(int row, int col, char symbol) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE || board[row][col] != ' ') {
        return 0;  // Invalid move
    }
    board[row][col] = symbol;
    return 1;  // Valid move
}

void handle_play_again(int server_socket_fd, int client_index, const char* response) {
    if (strncmp(response, YES_RESPONSE, strlen(YES_RESPONSE)) == 0) {
        reset_game();
        broadcast_message(server_socket_fd, "New game started!\n");
        broadcast_message(server_socket_fd, board_to_string());
        send_to_client(server_socket_fd, 0, TURN_MESSAGE);
    } else if (strncmp(response, NO_RESPONSE, strlen(NO_RESPONSE)) == 0) {
        broadcast_message(server_socket_fd, "Thanks for playing! Goodbye.\n");
        // Here you might want to implement logic to end the server or reset for new players
    } else {
        send_to_client(server_socket_fd, client_index, "Invalid response. Please answer 'yes' or 'no'.\n");
        send_to_client(server_socket_fd, client_index, PLAY_AGAIN_MESSAGE);
    }
}

void handle_client_message(int server_socket_fd, int client_index, char *message) {
    if (game_ended) {
        handle_play_again(server_socket_fd, client_index, message);
        return;
    }

    if (client_index != turn) {
        send_to_client(server_socket_fd, client_index, "It's not your turn.\n");
        return;
    }

    int row, col;
    if (sscanf(message, "%d %d", &row, &col) != 2 || !make_move(row-1, col-1, clients[client_index].symbol)) {
        send_to_client(server_socket_fd, client_index, "Invalid move. Try again.\n");
        send_to_client(server_socket_fd, client_index, TURN_MESSAGE);
        return;
    }

    broadcast_message(server_socket_fd, board_to_string());

    if (check_winner(clients[client_index].symbol)) {
        char win_message[BUFFER_SIZE];
        snprintf(win_message, BUFFER_SIZE, "Player %c wins!\n", clients[client_index].symbol);
        broadcast_message(server_socket_fd, win_message);
        game_ended = 1;
    } else if (is_board_full()) {
        broadcast_message(server_socket_fd, "It's a draw!\n");
        game_ended = 1;
    } else {
        turn = 1 - turn;
        send_to_client(server_socket_fd, turn, TURN_MESSAGE);
    }

    if (game_ended) {
        broadcast_message(server_socket_fd, PLAY_AGAIN_MESSAGE);
    }
}

void handle_client_activity(int server_socket_fd) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_storage client_addr;
    socklen_t addr_len = sizeof(client_addr);

    int bytes_received = recvfrom(server_socket_fd, buffer, BUFFER_SIZE - 1, 0,
                                  (struct sockaddr *)&client_addr, &addr_len);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        
        int client_index = find_client(&client_addr, addr_len);
        if (client_index == -1) {
            if (num_clients < MAX_CLIENTS) {
                client_index = add_new_client(&client_addr, addr_len);
                if (client_index != -1) {
                    handle_new_connection(server_socket_fd, client_index);
                }
            } else {
                const char *error_msg = "Game is full. Try again later.\n";
                sendto(server_socket_fd, error_msg, strlen(error_msg), 0,
                       (struct sockaddr *)&client_addr, addr_len);
            }
        } else {
            handle_client_message(server_socket_fd, client_index, buffer);
        }
    }
}

int main() {
    int server_socket_fd = setup_server();
    if (server_socket_fd == -1) {
        return 1;
    }

    printf("UDP Server is listening on port %s\n", MYPORT);
    fflush(stdout);

    while (1) {
        handle_client_activity(server_socket_fd);
    }

    close(server_socket_fd);
    return 0;
}