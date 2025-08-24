#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#define NV 20
#define NL 100 
char line[NL];
int jobCount = 0;

void prompt(void) {
    fprintf(stdout, "\nmsh> ");
    fflush(stdout);
}
void check_background_jobs(void) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("[#]+ Done                 pid=%d\n", pid);
        fflush(stdout);
    }
}

int main(int argk, char *argv[], char *envp[]) {
    pid_t pid;
    char *v[NV];
    char *sep = " \t\n";
    int i;

    while (1) {
        prompt();
        if (fgets(line, NL, stdin) == NULL) {
            if (feof(stdin)) exit(0);
            continue;
        }

        if (line[0] == '#' || line[0] == '\n') {
            check_background_jobs();
            continue;
        }

        v[0] = strtok(line, sep);
        for (i = 1; i < NV; i++) {
            v[i] = strtok(NULL, sep);
            if (v[i] == NULL) break;
        }

        if (v[0] == NULL) continue;
        if (strcmp(v[0], "cd") == 0) {
            if (v[1] == NULL) {
                fprintf(stderr, "cd: missing argument\n");
            } else if (chdir(v[1]) < 0) {
                perror("chdir");
            }
            check_background_jobs();
            continue;
        }
        int background = 0;
        if (i > 0 && v[i-1] && strcmp(v[i-1], "&") == 0) {
            background = 1;
            v[i-1] = NULL;
        }

        if ((pid = fork()) < 0) {
            perror("fork");
            continue;
        }
        else if (pid == 0) {
            execvp(v[0], v);
            perror("execvp");
            exit(1);
        }
        else {
            if (background) {
                jobCount++;
                printf("[%d] %d\n", jobCount, pid);
                fflush(stdout);
            } else {
                if (waitpid(pid, NULL, 0) < 0) {
                    perror("waitpid");
                }
            }
        }

        check_background_jobs();
    }
}
