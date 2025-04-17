#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdbool.h>
#include "boardgen.c"

#define GRID_SIZE 3
#define BOARD_SIZE (GRID_SIZE * GRID_SIZE)
#define MAX_CMD_LEN 512
#define MAX_INPUT 100

// Window positioning constants
#define WIN_X 50
#define WIN_Y 50
#define WIN_DX 380
#define WIN_DY 290

typedef struct {
    int read;
    int write;
} Pipe;

typedef struct {
    int up, down, left, right;
} Links;

typedef struct {
    Pipe* pipes;
    pid_t* pids;
    int board[BOARD_SIZE][BOARD_SIZE];
    int solution[BOARD_SIZE][BOARD_SIZE];
} Game;

// Helper functions
static int get_row(int block) { return block / GRID_SIZE; }
static int get_col(int block) { return block % GRID_SIZE; }
static int get_win_x(int block) { return WIN_X + get_col(block) * WIN_DX; }
static int get_win_y(int block) { return WIN_Y + get_row(block) * WIN_DY; }

void get_neighbors(int block, Links* links) {
    int row = get_row(block);
    int base = row * GRID_SIZE;
    
    // Horizontal links
    links->left = base + ((get_col(block) + 1) % GRID_SIZE);
    links->right = base + ((get_col(block) + 2) % GRID_SIZE);
    
    // Vertical links
    links->up = ((block + GRID_SIZE) % BOARD_SIZE);
    links->down = ((block + 2 * GRID_SIZE) % BOARD_SIZE);
    
    // Adjust if in same row
    if (get_row(links->up) == row)
        links->up = (block + 2 * GRID_SIZE) % BOARD_SIZE;
    if (get_row(links->down) == row)
        links->down = (block + GRID_SIZE) % BOARD_SIZE;
}

void create_xterm_cmd(char* cmd, int block, const Pipe* pipes, const Links* links) {
    snprintf(cmd, MAX_CMD_LEN,
        "env LANG=en_US.UTF-8 xterm "
        "-T \"Block %d\" "
        "-fa 'Liberation Mono' -fs 12 "
        "-geometry 30x10+%d+%d "
        "-u8 -b 2 -bg white -fg black "
        "-e ./block %d %d %d %d %d %d %d",
        block, get_win_x(block), get_win_y(block),
        block, pipes[block].read, pipes[block].write,
        pipes[links->left].write, pipes[links->right].write,
        pipes[links->up].write, pipes[links->down].write
    );
}

void print_help(void) {
    printf("\nCommands:\n"
           "n         - New game\n"
           "p b c d   - Place digit d [1-9] at cell c [0-8] of block b [0-8]\n"
           "s         - Show solution\n"
           "h         - Help\n"
           "q         - Quit\n\n"
           "Block layout:\n"
           "+---+---+---+\n"
           "| 0 | 1 | 2 |\n"
           "+---+---+---+\n"
           "| 3 | 4 | 5 |\n"
           "+---+---+---+\n"
           "| 6 | 7 | 8 |\n"
           "+---+---+---+\n\n");
}

void send_board(Game* game, bool show_solution) {
    const int (*board)[BOARD_SIZE] = show_solution ? game->solution : game->board;
    
    for (int block = 0; block < BOARD_SIZE; block++) {
        int row = get_row(block);
        int col = get_col(block);
        int stdout_backup = dup(STDOUT_FILENO);
        
        dup2(game->pipes[block].write, STDOUT_FILENO);
        printf("n");
        
        for (int i = 0; i < GRID_SIZE; i++)
            for (int j = 0; j < GRID_SIZE; j++)
                printf(" %d", board[row * GRID_SIZE + i][col * GRID_SIZE + j]);
        
        printf("\n");
        fflush(stdout);
        dup2(stdout_backup, STDOUT_FILENO);
        close(stdout_backup);
    }
}

void handle_move(Game* game, const char* input) {
    int block, cell, value;
    if (sscanf(input, "p %d %d %d", &block, &cell, &value) != 3 ||
        block < 0 || block >= BOARD_SIZE ||
        cell < 0 || cell >= BOARD_SIZE ||
        value < 1 || value > BOARD_SIZE) {
        printf("Invalid move\n");
        return;
    }
    
    int stdout_backup = dup(STDOUT_FILENO);
    dup2(game->pipes[block].write, STDOUT_FILENO);
    printf("p %d %d\n", cell, value);
    fflush(stdout);
    dup2(stdout_backup, STDOUT_FILENO);
    close(stdout_backup);
}

Game* init_game(void) {
    Game* game = calloc(1, sizeof(Game));
    game->pipes = calloc(BOARD_SIZE, sizeof(Pipe));
    game->pids = calloc(BOARD_SIZE, sizeof(pid_t));
    
    // Create pipes
    for (int i = 0; i < BOARD_SIZE; i++) {
        int pipefd[2];
        if (pipe(pipefd) != 0) {
            perror("Pipe creation failed");
            exit(1);
        }
        game->pipes[i].read = pipefd[0];
        game->pipes[i].write = pipefd[1];
    }
    
    // Launch processes
    for (int i = 0; i < BOARD_SIZE; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        }
        
        if (pid == 0) {
            Links links;
            get_neighbors(i, &links);
            
            char cmd[MAX_CMD_LEN];
            create_xterm_cmd(cmd, i, game->pipes, &links);
            
            system(cmd);
            exit(0);
        }
        
        game->pids[i] = pid;
    }
    
    return game;
}

void cleanup_game(Game* game) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        int stdout_backup = dup(STDOUT_FILENO);
        dup2(game->pipes[i].write, STDOUT_FILENO);
        printf("q\n");
        fflush(stdout);
        dup2(stdout_backup, STDOUT_FILENO);
        close(stdout_backup);
        
        waitpid(game->pids[i], NULL, 0);
        close(game->pipes[i].read);
        close(game->pipes[i].write);
    }
    
    free(game->pipes);
    free(game->pids);
    free(game);
}

int main(void) {
    Game* game = init_game();
    print_help();
    
    char input[MAX_INPUT];
    while (true) {
        printf("Foodoku> ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) break;
        
        switch (input[0]) {
            case 'h': print_help(); break;
            case 'n': 
                newboard(game->board, game->solution);
                send_board(game, false);
                break;
            case 'p': handle_move(game, input); break;
            case 's': send_board(game, true); break;
            case 'q':
                cleanup_game(game);
                printf("Goodbye!\n");
                return 0;
            default:
                if (!isspace(input[0])) printf("Unknown command\n");
        }
    }
    
    cleanup_game(game);
    return 0;
}