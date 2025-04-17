#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>


typedef enum RoundState {
    ROUND_ONGOING,
    ROUND_SUCCESS,
    ROUND_MISS,
    ROUND_OUT
} RoundState;

#define TEAM_SIZE_MAX 100
const int SUCCESS_THRESHOLD = 80;
#define STATUS_FILE "childpid.txt"
#define CONTROL_FILE "dummycpid.txt"


typedef struct SessionData {
    RoundState round_state;
    int member_count;
    int member_pids[TEAM_SIZE_MAX];
} SessionData;

static SessionData session = {ROUND_ONGOING, 0, {0}};

void display_status_and_signal(int sig_val) {
    
    switch (session.round_state) {
        case ROUND_ONGOING:
            printf("  ....   |");
            break;
        case ROUND_SUCCESS:
            printf("  CATCH  |");
            session.round_state = ROUND_ONGOING;
            break;
        case ROUND_MISS:
            printf("  MISS   |");
            session.round_state = ROUND_OUT;
            break;
        case ROUND_OUT:
            printf("         |");
            break;
    }
    fflush(stdout);

    pid_t current = getpid();
    int idx = 0;
    while (idx < session.member_count && session.member_pids[idx] != current) {
        idx++;
    }
    assert(idx != session.member_count);

    if (idx < session.member_count - 1) {
        kill(session.member_pids[idx + 1], SIGUSR1);
    } else {
        FILE* ctrl_file = fopen(CONTROL_FILE, "r");
        if (!ctrl_file) {
            perror("Control file error");
            exit(1);
        }
        pid_t supervisor;
        fscanf(ctrl_file, "%d", &supervisor);
        fclose(ctrl_file);
        kill(supervisor, SIGINT);
    }
}

void handle_throw(int sig_val) {
    assert(session.round_state == ROUND_ONGOING);
    int outcome = rand() % 100;
    
    if (outcome < SUCCESS_THRESHOLD) {
        session.round_state = ROUND_SUCCESS;
        kill(getppid(), SIGUSR1);
    } else {
        session.round_state = ROUND_MISS;
        kill(getppid(), SIGUSR2);
    }
}

void conclude_game(int sig_val) {
    if (session.round_state == ROUND_ONGOING) {
        pid_t winner = getpid();
        int pos = 0;
        
        while (pos < session.member_count && session.member_pids[pos] != winner) {
            pos++;
        }
        assert(pos != session.member_count);
        printf("+++ Child %d: Yay! I am the winner!\n", pos + 1);
        fflush(stdout);
    }
    exit(0);
}

int main(void) {
    srand(time(NULL) ^ (getpid() << 16));
    
    struct timespec init_delay = {1, 0};
    nanosleep(&init_delay, NULL);

    FILE* status = fopen(STATUS_FILE, "r");
    if (!status) {
        perror("Status file error");
        exit(1);
    }

    if (fscanf(status, "%d", &session.member_count) != 1) {
        fclose(status);
        fprintf(stderr, "Invalid member count\n");
        exit(1);
    }

    for (int i = 0; i < session.member_count; i++) {
        if (fscanf(status, "%d", &session.member_pids[i]) != 1) {
            fclose(status);
            fprintf(stderr, "Invalid PID at position %d\n", i);
            exit(1);
        }
    }
    fclose(status);

    struct sigaction handlers[3];
    handlers[0].sa_handler = display_status_and_signal;
    handlers[1].sa_handler = handle_throw;
    handlers[2].sa_handler = conclude_game;
    
    for (int i = 0; i < 3; i++) {
        sigemptyset(&handlers[i].sa_mask);
        handlers[i].sa_flags = 0;
    }
    
    sigaction(SIGUSR1, &handlers[0], NULL);
    sigaction(SIGUSR2, &handlers[1], NULL);
    sigaction(SIGINT, &handlers[2], NULL);

    while (1) {
        pause();
    }

    return 0;
}