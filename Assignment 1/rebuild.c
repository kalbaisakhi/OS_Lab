#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define FOODEP "foodep.txt"
#define DONE "done.txt"
#define MAX_LINE 1024
#define MAX_DEPS 20

void init_done(int n) {
    FILE* fp = fopen(DONE, "w");
    if (!fp) {
        fprintf(stderr, "Error\n");
        exit(1);
    }
    for (int i = 0; i < n; i++) fprintf(fp, "0");
    fclose(fp);
}

void read_done(int* visited, int n) {
    FILE* fp = fopen(DONE, "r");
    if (!fp) {
        fprintf(stderr, "Error\n");
        exit(1);
    }
    for (int i = 0; i < n; i++) {
        int c = fgetc(fp);
        if (c == EOF) {
            fclose(fp);
            fprintf(stderr, "Error\n");
            exit(1);
        }
        visited[i] = c - '0';
    }
    fclose(fp);
}

void write_done(int* visited, int n) {
    FILE* fp = fopen(DONE, "w");
    if (!fp) {
        fprintf(stderr, "Error\n");
        exit(1);
    }
    for (int i = 0; i < n; i++) fprintf(fp, "%d", visited[i]);
    fclose(fp);
}

int get_n() {
    FILE* fp = fopen(FOODEP, "r");
    if (!fp) {
        fprintf(stderr, "Error\n");
        exit(1);
    }
    int n;
    if (fscanf(fp, "%d", &n) != 1) {
        fclose(fp);
        fprintf(stderr, "Error\n");
        exit(1);
    }
    fclose(fp);
    return n;
}

void get_dependencies(int u, int* deps, int* num_deps) {
    FILE* fp = fopen(FOODEP, "r");
    if (!fp) {
        fprintf(stderr, "Error\n");
        exit(1);
    }
    char line[MAX_LINE];
    fgets(line, MAX_LINE, fp);
    *num_deps = 0;
    while (fgets(line, MAX_LINE, fp)) {
        char* p = strtok(line, ":");
        if (p) {
            int curr_foo = atoi(p);
            if (curr_foo == u) {
                p = strtok(NULL, " \n");
                while (p) {
                    deps[(*num_deps)++] = atoi(p);
                    p = strtok(NULL, " \n");
                }
                break;
            }
        }
    }
    fclose(fp);
}

void rebuild_foodule(int u, int is_child) {
    int n = get_n();
    int* visited = (int*)malloc(n * sizeof(int));
    int deps[MAX_DEPS];
    int num_deps = 0;
    if (!is_child) init_done(n);
    get_dependencies(u, deps, &num_deps);
    for (int i = 0; i < num_deps; i++) {
        read_done(visited, n);
        if (!visited[deps[i] - 1]) {
            pid_t pid = fork();
            if (pid < 0) {
                free(visited);
                fprintf(stderr, "Error\n");
                exit(1);
            }
            if (pid == 0) {
                char dep_str[32];
                snprintf(dep_str, sizeof(dep_str), "%d", deps[i]);
                execlp("./rebuild", "./rebuild", dep_str, "1", NULL);
                fprintf(stderr, "Error\n");
                exit(1);
            } else {
                waitpid(pid, NULL, 0);
            }
        }
    }
    read_done(visited, n);
    if (!visited[u - 1]) {
        visited[u - 1] = 1;
        write_done(visited, n);
        printf("foo%d rebuilt", u);
        if (num_deps > 0) {
            int first = 1;
            for (int i = 0; i < num_deps; i++) {
                if (visited[deps[i] - 1]) {
                    if (first) {
                        printf(" from");
                        first = 0;
                    }
                    printf(" foo%d", deps[i]);
                    if (i < num_deps - 1) {
                        int j = i + 1;
                        while (j < num_deps && !visited[deps[j] - 1]) j++;
                        if (j < num_deps) printf(",");
                    }
                }
            }
        }
        printf("\n");
    }
    free(visited);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error\n");
        exit(1);
    }
    int foodule = atoi(argv[1]);
    int is_child = (argc > 2);
    rebuild_foodule(foodule, is_child);
    return 0;
}