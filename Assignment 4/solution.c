#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define ROW_CONFLICT 2
#define COL_CONFLICT 3
#define BOLD "\033[1m"
#define BOLD_RED "\033[1;31m"
#define RESET "\033[0m"
int fd_in, fd_out, rd1_out, rd2_out, cd1_out, cd2_out, fd1copy;
void clear_screen()
{
    printf("\033[H\033[J"); // ANSI escape sequence to clear screen
    fflush(stdout);         // Ensure immediate execution
}

void print_grid(int arr[3][3],int fixed[3][3])
{
    printf("\n      +---+---+---+\n");
    for (int i = 0; i < 3; i++)
    {
        printf("      |");
        for (int j = 0; j < 3; j++)
        {
            if (fixed[i][j] != 0)
                printf(BOLD " %d " RESET "|", arr[i][j]);
            else if (arr[i][j] != 0)
                printf(" %d |", arr[i][j]);
            else
                printf("   |");
        }
        printf("\n      +---+---+---+\n");
    }
}
int block_conflict(int arr[3][3], int x)
{
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (arr[i][j] == x)
            {
                return 1;
            }
        }
    }
    return 0;
}

int row_conflict(int c, int d)
{
    int row = c / 3;
    fd1copy = dup(1);
    close(1);
    dup(rd1_out);
    printf("r %d %d %d\n", row, d, fd_out);
    fflush(stdout);
    int conflict;

    scanf("%d", &conflict);
    if (conflict == ROW_CONFLICT)
    {
        close(1);
        dup(fd1copy);
        close(fd1copy);
        return 1;
    }
    close(1);
    dup(rd2_out);
    printf("r %d %d %d\n", row, d, fd_out);
    fflush(stdout);
    scanf("%d", &conflict);
    close(1);
    dup(fd1copy);
    close(fd1copy);
    if (conflict == ROW_CONFLICT)
        return 1;
    return 0;
}

int col_conflict(int c, int d)
{
    int col = c % 3;
    fd1copy = dup(1);
    close(1);
    dup(cd1_out);
    printf("c %d %d %d\n", col, d, fd_out);
    fflush(stdout);
    int conflict;
    scanf("%d", &conflict);
    if (conflict == COL_CONFLICT)
    {
        close(1);
        dup(fd1copy);
        close(fd1copy);

        return 1;
    }
    close(1);
    dup(cd2_out);
    printf("c %d %d %d\n", col, d, fd_out);
    fflush(stdout);
    scanf("%d", &conflict);
    close(1);
    dup(fd1copy);
    close(fd1copy);
    if (conflict == COL_CONFLICT)
        return 1;
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 8)
    {
        fprintf(stderr, "All arguments not received\n");
        exit(1);
    }

    int block_no = atoi(argv[1]);
    fd_in = atoi(argv[2]);
    fd_out = atoi(argv[3]);
    rd1_out = atoi(argv[4]);
    rd2_out = atoi(argv[5]);
    cd1_out = atoi(argv[6]);
    cd2_out = atoi(argv[7]);
    printf("Block %d ready\n", block_no);
    close(0);
    dup(fd_in);
    char ch;
    int A[3][3], B[3][3];
    while (1)
    {
        if (scanf(" %c", &ch) > 0)
        {
            if (ch == 'n')
            {
                clear_screen();
                for (int i = 0; i < 3; i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        int tmp;
                        scanf("%d", &tmp);
                        A[i][j] = B[i][j] = tmp;
                    }
                }
                print_grid(A,A);
            }
            else if (ch == 'p')
            {

                int d, c;
                scanf("%d", &c);
                scanf("%d", &d);
                int i = c / 3, j = c % 3;
                if (A[i][j] != 0)
                {
                    printf(BOLD_RED "Readonly Cell\n" RESET);
                    sleep(2);
                }
                else if (block_conflict(B, d))
                {
                    printf(BOLD_RED "Block Conflict\n" RESET);
                    sleep(2);
                }
                else if (row_conflict(c, d))
                {
                    printf(BOLD_RED "Row Conflict\n" RESET);
                    sleep(2);
                }
                else if (col_conflict(c, d))
                {
                    printf(BOLD_RED "Column Conflict\n" RESET);
                    sleep(2);
                }
                else
                {
                    B[i][j] = d;
                }

                clear_screen();
                print_grid(B,A);
            }
            else if (ch == 'r')
            {
                int i, d;
                scanf("%d", &i);
                scanf("%d", &d);
                int pfd1_req;
                scanf("%d", &pfd1_req);
                fd1copy = dup(1);
                close(1);
                dup(pfd1_req);
                int conflict = 0;
                for (int j = 0; j < 3; j++)
                {
                    if (B[i][j] == d)
                    {
                        conflict = ROW_CONFLICT;
                        break;
                    }
                }
                printf("%d\n", conflict);
                fflush(stdout);
                close(1);
                dup(fd1copy);
                close(fd1copy);
            }
            else if (ch == 'c')
            {
                int j, d;
                scanf("%d", &j);
                scanf("%d", &d);
                int pfd1_req;
                scanf("%d", &pfd1_req);
                fd1copy = dup(1);
                close(1);
                dup(pfd1_req);
                int conflict = 0;
                for (int i = 0; i < 3; i++)
                {
                    if (B[i][j] == d)
                    {
                        conflict = COL_CONFLICT;
                        break;
                    }
                }
                printf("%d\n", conflict);
                fflush(stdout);
                close(1);
                dup(fd1copy);
                close(fd1copy);
            }
            else if (ch == 'q')
            {
                printf("Bye...\n");
                sleep(2);
                break;
            }
        }
    }

    return 0;
}