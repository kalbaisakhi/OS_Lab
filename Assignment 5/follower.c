#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <sys/wait.h>

#define MAX_FOLLOWERS 100

int main(int argc, char *argv[]) {
    int nf = 1;
    
    if (argc > 1) {
        nf = atoi(argv[1]);
    }

    key_t key = ftok("/", 1234);
    int shmid = shmget(key, 0, 0666);
    if(shmid<0){
        fprintf(stderr, "Error : could not find shared memory. Leader not running ?\n");
        exit(1);
    }
    int *M = (int *)shmat(shmid, NULL, 0);

    for (int proc = 0; proc < nf; proc++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            srand((unsigned int)time(NULL) ^ getpid());
            usleep(50000);
            
            if (M[1] >= M[0]) {
                printf("follower error: %d followers have already joined\n", M[0]);
                fflush(stdout);
                shmdt(M);
                exit(0);
            }

            int i = M[1] + 1;
            M[1] = i;
            printf("follower %d joins\n", i);
            fflush(stdout);

            while (1) {
                while (M[2] != i && M[2] != -i) {
                    if (M[2] < 0 && abs(M[2]) != i) {
                        if (abs(M[2]) > i) {
                        }
                    }
                    usleep(50000);
                }

                if (M[2] == i) {
                    M[3 + i] = (rand() % 9) + 1;

                    if (i < M[0]) {
                        M[2] = i + 1;
                    } else {
                        M[2] = 0;
                    }
                } else if (M[2] == -i) {
                    printf("follower %d leaves\n", i);
                    fflush(stdout);

                    if (i < M[0]) {
                        M[2] = -(i + 1);
                    } else {
                        M[2] = 0;
                    }
                    break;
                }
            }

            shmdt(M);
            exit(0);
        }
    }

    for (int proc = 0; proc < nf; proc++) {
        wait(NULL);
    }

    shmdt(M);
    return 0;
}