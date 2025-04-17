#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>

#define P_ACTIVE 1
#define P_OUT 7
#define P_MAX 100



typedef struct {
    pid_t processID;
    int statusFlag;
} ActivePlayer;

typedef struct {
    ActivePlayer* roster;
    int totalPlayers;
    int activeCount;
    int currentIndex;
    int initialized;
} GameMaster;

static GameMaster gState = {NULL, 0, 0, 0, 0};


void proceedNext(int signalVal) {
    
}

void eliminate(int signalVal) {
    if (gState.roster[gState.currentIndex].statusFlag == P_ACTIVE) {
        gState.activeCount--;
        gState.roster[gState.currentIndex].statusFlag = P_OUT;
    }
}

void renderField(void) {
    printf("|    ");
    for (int i = 0; i < gState.totalPlayers; i++) {
        printf(i < 9 ? "    %d    |" : "   %d    |", i + 1);
    }
    printf("\n");
    fflush(stdout);
}

void setUpSignals(void) {
    struct sigaction actionNextRound = {0}, actionEliminate = {0};
    actionNextRound.sa_handler = proceedNext;
    actionEliminate.sa_handler = eliminate;
    sigaction(SIGUSR1, &actionNextRound, NULL);
    sigaction(SIGUSR2, &actionEliminate, NULL);
}

void spawnPlayers(void) {
    FILE* logFile = fopen("childpid.txt", "w");
    if (!logFile) {
        perror("Log error");
        exit(EXIT_FAILURE);
    }

    fprintf(logFile, "%d\n", gState.totalPlayers);

    for (int i = 0; i < gState.totalPlayers; i++) {
        pid_t newProc = fork();
        if (newProc < 0) {
            perror("Fork error");
            exit(EXIT_FAILURE);
        } else if (newProc == 0) {
            execl("./child", "./child", NULL);
            perror("Exec error");
            exit(EXIT_FAILURE);
        }
        gState.roster[i].processID = newProc;
        gState.roster[i].statusFlag = P_ACTIVE;
        fprintf(logFile, "%d\n", newProc);
    }
    fclose(logFile);
}

void playRounds(void) {
    while (gState.activeCount > 1) {
        if (gState.roster[gState.currentIndex].statusFlag == P_ACTIVE) {
            printf("+_______________________________________________________________________________________________________+\n");
            printf("|    ");
            fflush(stdout);

            kill(gState.roster[gState.currentIndex].processID, SIGUSR2);
            pause();  /* Wait for the signal that marks the current player out. */

            pid_t dummyProc = fork();
            if (dummyProc < 0) {
                perror("Fork error");
                exit(EXIT_FAILURE);
            } else if (dummyProc == 0) {
                execl("./dummy", "./dummy", NULL);
                perror("Exec error");
                exit(EXIT_FAILURE);
            }

            FILE* auxLog = fopen("dummycpid.txt", "w");
            if (!auxLog) {
                perror("Log error");
                exit(EXIT_FAILURE);
            }
            fprintf(auxLog, "%d\n", dummyProc);
            fclose(auxLog);

            kill(gState.roster[0].processID, SIGUSR1);
            waitpid(dummyProc, NULL, 0);
            printf("\n");
            fflush(stdout);
        }
        gState.currentIndex = (gState.currentIndex + 1) % gState.totalPlayers;
    }
}

void cleanupGame(void) {
    for (int i = 0; i < gState.totalPlayers; i++) {
        kill(gState.roster[i].processID, SIGINT);
    }
    for (int i = 0; i < gState.totalPlayers; i++) {
        waitpid(gState.roster[i].processID, NULL, 0);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <count>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    gState.totalPlayers = atoi(argv[1]);
    if (gState.totalPlayers < 1 || gState.totalPlayers > P_MAX) {
        fprintf(stderr, "Count must be 1-%d\n", P_MAX);
        exit(EXIT_FAILURE);
    }

    gState.roster = malloc(sizeof(ActivePlayer) * gState.totalPlayers);
    if (!gState.roster) {
        perror("Memory error");
        exit(EXIT_FAILURE);
    }

    spawnPlayers();
    printf("Parent: %d child processes created\n", gState.totalPlayers);
    printf("Parent: Waiting for child processes to read child database\n");
    fflush(stdout);

    sleep(2);
    renderField();

    gState.currentIndex = 0;
    gState.activeCount = gState.totalPlayers;

    setUpSignals();
    playRounds();

    printf("+________________________________________________________________________________________________________+\n");
    renderField();
    printf("\n");

    cleanupGame();
    free(gState.roster);

    return 0;
}