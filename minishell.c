#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#define NV 20 /* max number of command tokens */
#define NL 100 /* input buffer size */
char line[NL]; /* command input buffer */

// ADD: track full command line for background jobs
char fullcmd[NL];

// ADD: job struct
int jobCount = 0;
typedef struct {
    int jobid;
    pid_t pid;
    char cmd[NL];
} Job;
Job jobs[100];
int jobIndex = 0;

/*
shell prompt
*/
void prompt(void)
{
// ## REMOVE THIS 'fprintf' STATEMENT BEFORE SUBMISSION
fprintf(stdout, "\n msh> ");
fflush(stdout);
}

// ADD: check background jobs
void check_background_jobs(void) {
    int status;
    pid_t pid;
    for (int j = 0; j < jobIndex; j++) {
        if (jobs[j].pid > 0) {
            pid = waitpid(jobs[j].pid, &status, WNOHANG);
            if (pid > 0) {
                printf("[%d]+ Done                 %s\n",
                       jobs[j].jobid, jobs[j].cmd);
                fflush(stdout);
                jobs[j].pid = -1; // mark finished
            }
        }
    }
}

/* argk - number of arguments */
/* argv - argument vector from command line */
/* envp - environment pointer */
int main(int argk, char *argv[], char *envp[])
{
int frkRtnVal; /* value returned by fork sys call */
char *v[NV]; /* array of pointers to command line tokens */
char *sep = " \t\n"; /* command line token separators */
int i; /* parse index */
/* prompt for and process one command line at a time */
while (1) { /* do Forever */
prompt();
fgets(line, NL, stdin);
fflush(stdin);

// ADD: save full command string
strncpy(fullcmd, line, NL);
fullcmd[NL-1] = '\0';

// This if() required for gradescope
if (feof(stdin)) { /* non-zero on EOF */
exit(0);
}
if (line[0] == '#' || line[0] == '\n' || line[0] == '\000'){
continue; /* to prompt */
}
v[0] = strtok(line, sep);
for (i = 1; i < NV; i++) {
v[i] = strtok(NULL, sep);
if (v[i] == NULL){
break;
}
}
/* assert i is number of tokens + 1 */

// ADD: built-in cd
if (v[0] && strcmp(v[0], "cd") == 0) {
    if (v[1] == NULL) {
        fprintf(stderr, "cd: missing argument\n");
    } else if (chdir(v[1]) < 0) {
        perror("chdir");
    }
    check_background_jobs();
    continue;
}

// ADD: detect background '&'
int background = 0;
if (i > 0 && v[i-1] && strcmp(v[i-1], "&") == 0) {
    background = 1;
    v[i-1] = NULL; // mark end of args
}

/* fork a child process to exec the command in v[0] */
switch (frkRtnVal = fork()) {
case -1: /* fork returns error to parent process */
{
perror("fork"); // ADD
break;
}
case 0: /* code executed only by child process */
{
execvp(v[0], v);
perror("execvp"); // ADD
exit(1); // ADD
}
default: /* code executed only by parent process */
{
if (background) {
    jobCount++;
    jobs[jobIndex].jobid = jobCount;
    jobs[jobIndex].pid = frkRtnVal;

    // ADD: save command string (remove &)
    snprintf(jobs[jobIndex].cmd, NL, "%s", fullcmd);
    char *amp = strchr(jobs[jobIndex].cmd, '&');
    if (amp) *amp = '\0';

    jobIndex++;
    printf("[%d] %d\n", jobCount, frkRtnVal);
    fflush(stdout);
} else {
    if (wait(0) < 0) { // existing line kept
        perror("wait"); // ADD
    }
    // REMOVE PRINTF STATEMENT BEFORE SUBMISSION
    printf("%s done \n", v[0]);
}
break;
}
} /* switch */

// ADD: check background jobs each loop
check_background_jobs();

} /* while */
} /* main */
