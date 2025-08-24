/*********************************************************************
   Program  : miniShell                   Version    : 1.3
 --------------------------------------------------------------------
   skeleton code for linix/unix/minix command line interpreter
 --------------------------------------------------------------------
   File			: minishell.c
   Compiler/System	: gcc/linux

********************************************************************/

#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#define NV 20			/* max number of command tokens */
#define NL 100			/* input buffer size */
#define JOBSMAXIMUM 100		/* max number of background jobs */
char line[NL];			/* command input buffer */

/*structure to track background jobs */
typedef struct {
    pid_t pid;
    int jobNumber;
    char command[NL];
    int activation;
} Job;

Job jobs[JOBSMAXIMUM];
int jobCounter = 0;

/*initialize job array*/
void jobsArr(void)
{
    int i;
    for (i = 0; i < JOBSMAXIMUM; i++) {
        jobs[i].activation = 0;
        jobs[i].pid = 0;
        jobs[i].jobNumber = 0;
    }
}

/*add a background job*/
void jobsAddition(pid_t pid, const char *cmd)
{
    int i;
    jobCounter++;
    for (i = 0; i < JOBSMAXIMUM; i++) {
        if (!jobs[i].activation) {
            jobs[i].pid=pid;
            jobs[i].jobNumber=jobCounter;
            strncpy(jobs[i].command,cmd,NL - 1);
            jobs[i].command[NL-1] ='\0';
            jobs[i].activation=1;
            printf("[%d] %d\n",jobs[i].jobNumber, pid);
            fflush(stdout);
            break;
        }
    }
}

/*check for completed background jobs*/
void backgroundJobsChecking(void)
{
    int i, status;
    pid_t pid;
    for (i = 0; i < JOBSMAXIMUM; i++) {
        if (jobs[i].activation) {
            pid = waitpid(jobs[i].pid, &status, WNOHANG);
            if (pid == -1) {
                perror("waitpid");
                jobs[i].activation = 0;
            } else if (pid == jobs[i].pid) {
                printf("[%d]+ Done                    %s\n", 
                       jobs[i].jobNumber, jobs[i].command);
                fflush(stdout);
                jobs[i].activation = 0;
            }
        }
    }
}

/*
	shell prompt
 */
void prompt(void)
{
    /* Removed fprintf statement as required for submission */
    /* No output for submission version */
}

/*SIGCHLD to reap zombie children*/
void sigchld_handler(int sig)
{
    int saved_errno = errno;
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    }
    errno = saved_errno;
}

/* argk - number of arguments */
/* argv - argument vector from command line */
/* envp - environment pointer */
int main(int argk, char *argv[], char *envp[])
{
    int frkRtnVal;		/* value returned by fork sys call */
    char *v[NV];		/* array of pointers to command line tokens */
    char *sep = " \t\n";	/* command line token separators */
    int i;			/* parse index */
    int background;		/* flag for background execution */
    char copy[NL];		/* copy of command for background jobs */
    
    /*initialize job tracking*/
    jobsArr();
    
    /*set up SIGCHLD handler*/
    signal(SIGCHLD, sigchld_handler);
    
    /* prompt for and process one command line at a time */
    while (1) {			/* do Forever */
        /*check for completed background jobs*/
        backgroundJobsChecking();
        prompt();
        if (fgets(line, NL, stdin) == NULL) {
            if (feof(stdin)) {
                exit(0);
            }
            perror("fgets");
            continue;
        }
        /* This if() required for gradescope */
        if (feof(stdin)) {		/* non-zero on EOF */
            exit(0);
        }
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\000') {
            continue;			/* to prompt */
        }
        
        /* Make a copy of the command line for background job tracking */
        strncpy(copy, line, NL - 1);
        copy[NL - 1] = '\0';
        
        /* Remove trailing newline from copy */
        size_t length = strlen(copy);
        if (length > 0 && copy[length - 1] == '\n') {
            copy[length - 1] = '\0';
        }
        /*parse the command line*/
        v[0] = strtok(line, sep);
        for (i = 1; i < NV; i++) {
            v[i] = strtok(NULL, sep);
            if (v[i] == NULL) {
                break;
            }
        }
        /* assert i is number of tokens */
        /*check for background execution */
        background = 0;
        if (i > 1 && strcmp(v[i - 1], "&") == 0) {
            background = 1;
            v[i - 1] = NULL;	/* Remove & from arguments */
            i--;
            
            /* remove and from copy for display */
            length = strlen(copy);
            if (length > 0) {
                char *amp = strrchr(copy, '&');
                if (amp != NULL) {
                    *amp = '\0';
                    /*Trim trailing spaces*/
                    while (amp > copy && *(amp - 1) == ' ') {
                        *(--amp) = '\0';
                    }
                }
            }
        }
        
        /*Check if command is empty after parsing*/
        if (v[0] == NULL) {
            continue;
        }
        
        /*Handle built-in cd command*/
        if (strcmp(v[0], "cd") == 0) {
            if (v[1] == NULL) {
                char *home = getenv("HOME");
                if (home != NULL) {
                    if (chdir(home) == -1) {
                        perror("chdir");
                    }
                } else {
                    fprintf(stderr, "cd: HOME not set\n");
                }
            } else {
                if (chdir(v[1]) == -1) {
                    perror("chdir");
                }
            }
            continue;
        }
        
        /*fork a child process to exec the command*/
        switch (frkRtnVal = fork()) {
            case -1:			/* fork returns error to parent process */
                perror("fork");
                break;
            case 0:			/* code executed only by child process */
                if (execvp(v[0], v) == -1) {
                    perror("execvp");
                    exit(EXIT_FAILURE);	/* Properly terminate child on exec failure */
                }
                break;
                
            default:			/* code executed only by parent process */
                if (background) {
                    /* Add to background job list */
                    jobsAddition(frkRtnVal, copy);
                } else {
                    /* Wait for foreground process */
                    if (waitpid(frkRtnVal, NULL, 0) == -1) {
                        perror("waitpid");
                    }
                    /* Removed printf statement as required for submission */
                }
                break;
        }				/* switch */
    }				/* while */
    
    return 0;
}				/* main */