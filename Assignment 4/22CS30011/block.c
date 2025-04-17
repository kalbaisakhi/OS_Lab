#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#define SIZE 3
#define EMPTY 0

typedef struct {
    int in;
    int out;
    int h[2];
    int v[2];
} Pipes;

typedef struct {
    int fixed[SIZE][SIZE];
    int current[SIZE][SIZE];
    int id;
    Pipes pipes;
} Block;

static void clear_screen(void) {
    printf("\033[2J\033[H");
}

static void print_cell(int value) {
    printf(" %c ", value == EMPTY ? ' ' : '0' + value);
}

static void show_board(const Block* block) {
    clear_screen();
    printf("Block %d\n", block->id);
    
    // Top border
    printf("┌───┬───┬───┐\n");
    
    for (int r = 0; r < SIZE; r++) {
        printf("│");
        for (int c = 0; c < SIZE; c++) {
            print_cell(block->current[r][c]);
            printf("│");
        }
        printf("\n");
        
        if (r < SIZE - 1) {
            printf("├───┼───┼───┤\n");
        }
    }
    
    // Bottom border
    printf("└───┴───┴───┘\n");
    fflush(stdout);
}

static void show_error(const Block* block, const char* msg) {
    printf("%s\n", msg);
    fflush(stdout);
    sleep(2);
    show_board(block);
}

static bool is_fixed(const Block* block, int row, int col) {
    return block->fixed[row][col] != EMPTY;
}

static bool has_conflict(const Block* block, int value) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (block->current[i][j] == value) {
                return true;
            }
        }
    }
    return false;
}

static bool check_line(const Block* block, char dir, int idx, int value, int pipe1, int pipe2) {
    dprintf(pipe1, "%c %d %d %d\n", dir, idx, value, block->pipes.out);
    dprintf(pipe2, "%c %d %d %d\n", dir, idx, value, block->pipes.out);
    
    int r1, r2;
    FILE* in = fdopen(block->pipes.in, "r");
    fscanf(in, "%d %d", &r1, &r2);
    
    return !(r1 || r2);
}

static void validate_line(const Block* block, char dir, int idx, int value, int sender) {
    bool conflict = false;
    for (int i = 0; i < SIZE; i++) {
        int cell = (dir == 'h') ? block->current[idx][i] : block->current[i][idx];
        if (cell == value) {
            conflict = true;
            break;
        }
    }
    dprintf(sender, "%d\n", conflict ? 1 : 0);
}

static void handle_new(Block* block, FILE* in) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            fscanf(in, "%d", &block->fixed[i][j]);
            block->current[i][j] = block->fixed[i][j];
        }
    }
    show_board(block);
}

static void handle_place(Block* block, FILE* in) {
    int pos, value;
    fscanf(in, "%d %d", &pos, &value);
    int row = pos / SIZE, col = pos % SIZE;
    
    if (is_fixed(block, row, col)) {
        show_error(block, "Read-only cell");
        return;
    }
    
    if (has_conflict(block, value) && block->current[row][col] != value) {
        show_error(block, "Block conflict");
        return;
    }
    
    if (!check_line(block, 'h', row, value, block->pipes.h[0], block->pipes.h[1])) {
        show_error(block, "Row conflict");
        return;
    }
    
    if (!check_line(block, 'v', col, value, block->pipes.v[0], block->pipes.v[1])) {
        show_error(block, "Column conflict");
        return;
    }
    
    block->current[row][col] = value;
    show_board(block);
}

static void process_command(Block* block, char cmd, FILE* in) {
    switch (cmd) {
        case 'n':
            handle_new(block, in);
            break;
        case 'p':
            handle_place(block, in);
            break;
        case 'h':
        case 'v': {
            int idx, val, sender;
            fscanf(in, "%d %d %d", &idx, &val, &sender);
            validate_line(block, cmd, idx, val, sender);
            break;
        }
        case 'q':
            printf("Bye from block %d...\n", block->id);
            fflush(stdout);
            sleep(1);
            exit(0);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 8) {
        fprintf(stderr, "Usage: %s blockId in out h1 h2 v1 v2\n", argv[0]);
        return 1;
    }
    
    Block block = {0};
    block.id = atoi(argv[1]);
    block.pipes = (Pipes){
        .in = atoi(argv[2]),
        .out = atoi(argv[3]),
        .h = {atoi(argv[4]), atoi(argv[5])},
        .v = {atoi(argv[6]), atoi(argv[7])}
    };
    
    dup2(block.pipes.in, STDIN_FILENO);
    FILE* in = fdopen(block.pipes.in, "r");
    show_board(&block);
    
    char cmd;
    while (fscanf(in, " %c", &cmd) != EOF) {
        process_command(&block, cmd, in);
        fflush(stdout);
    }
    
    return 0;
}