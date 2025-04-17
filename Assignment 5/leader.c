#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#define MAX_FOLLOWERS 100
#define MAX_SUM_TRACK 100000

int main(int argc, char *argv[]) {
    int n = 10;
    static int sumSeen[MAX_SUM_TRACK];
    
    if (argc > 1) {
        n = atoi(argv[1]);
    }

    key_t key = ftok("/", 1234);
    int shmid = shmget(key, (4 + n) * sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
    int *M = (int *)shmat(shmid, NULL, 0);

    M[0] = n;   
    M[1] = 0;   
    M[2] = 0;   
    
    for (int i = 3; i < 3 + n + 1; i++) {
        M[i] = 0;
    }

    printf("Wait for the moment.\n");

    while (M[1] < n) {
        usleep(100000);
    }

    memset(sumSeen, 0, sizeof(sumSeen));

    while (1) {
        if (M[2] == 0) {
            srand((unsigned int)time(NULL) ^ getpid());
            M[3] = (rand() % 99) + 1;
            M[2] = 1;
        }

        while (M[2] != 0 && M[2] >= 0) {
            if (M[2] < 0) {
                goto terminate;
            }
            usleep(100000);
        }

        if (M[2] < 0) {
            break;
        }

        int sum = 0;
        printf("%d", M[3]);
        sum += M[3];
        
        for (int i = 4; i < 4 + n; i++) {
            printf(" + %d", M[i]);
            sum += M[i];
        }
        printf(" = %d\n", sum);

        if (sum > 0 && sum < MAX_SUM_TRACK && sumSeen[sum]) {
            M[2] = -1;
        } else {
            if (sum >= 0 && sum < MAX_SUM_TRACK) {
                sumSeen[sum] = 1;
            }
        }
    }

terminate:
    while (1) {
        if (M[2] == 0 || M[2] == -(n + 1)) {
            break;
        }
        usleep(100000);
    }

    shmdt(M);
    shmctl(shmid, IPC_RMID, NULL);
    return 0;
}