/*********************************************************************
   Program  : miniShell                   Version    : 1.3
 --------------------------------------------------------------------
   skeleton code for linix/unix/minix command line interpreter
 --------------------------------------------------------------------
   File			: minishell.c
   Compiler/System	: gcc/linux

********************************************************************/

#define _POSIX_C_SOURCE 200809L  /* Enable POSIX features */

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
#define MAX_JOBS 100		/* max number of background jobs */

char line[NL];			/* command input buffer */

/* Structure to track background jobs */
typedef struct {
    pid_t pid;
    int job_num;
    char command[NL];
    int active;
} Job;

Job jobs[MAX_JOBS];
int job_counter = 0;

/*
	Initialize job array
 */
void init_jobs(void)
{
    int i;
    for (i = 0; i < MAX_JOBS; i++) {
        jobs[i].active = 0;
        jobs[i].pid = 0;
        jobs[i].job_num = 0;
    }
}

/*
	Add a background job to the job list
 */
void add_job(pid_t pid, const char *cmd)
{
    int i;
    job_counter++;
    
    for (i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].active) {
            jobs[i].pid = pid;
            jobs[i].job_num = job_counter;
            strncpy(jobs[i].command, cmd, NL - 1);
            jobs[i].command[NL - 1] = '\0';
            jobs[i].active = 1;
            printf("[%d] %d\n", jobs[i].job_num, pid);
            fflush(stdout);
            break;
        }
    }
}

/*
	Check for completed background jobs
 */
void check_background_jobs(void)
{
    int i, status;
    pid_t pid;
    
    for (i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active) {
            pid = waitpid(jobs[i].pid, &status, WNOHANG);
            if (pid == -1) {
                perror("waitpid");
                jobs[i].active = 0;
            } else if (pid == jobs[i].pid) {
                printf("[%d]+ Done                    %s\n", 
                       jobs[i].job_num, jobs[i].command);
                fflush(stdout);
                jobs[i].active = 0;
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

/*
	Handle SIGCHLD to reap zombie children
 */
void sigchld_handler(int sig)
{
    int saved_errno = errno;  /* Save errno */
    pid_t pid;
    int status;
    
    /* Reap all available zombie children */
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        /* Child reaped */
    }
    
    errno = saved_errno;  /* Restore errno */
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
    char cmd_copy[NL];		/* copy of command for background jobs */
    
    /* Initialize job tracking */
    init_jobs();
    
    /* Set up SIGCHLD handler to prevent zombies */
    signal(SIGCHLD, sigchld_handler);
    
    /* prompt for and process one command line at a time */
    while (1) {			/* do Forever */
        /* Check for completed background jobs */
        check_background_jobs();
        
        prompt();
        if (fgets(line, NL, stdin) == NULL) {
            if (feof(stdin)) {
                exit(0);  /* Exit cleanly on EOF */
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
        strncpy(cmd_copy, line, NL - 1);
        cmd_copy[NL - 1] = '\0';
        
        /* Remove trailing newline from cmd_copy */
        size_t len = strlen(cmd_copy);
        if (len > 0 && cmd_copy[len - 1] == '\n') {
            cmd_copy[len - 1] = '\0';
        }
        
        /* Parse the command line */
        v[0] = strtok(line, sep);
        for (i = 1; i < NV; i++) {
            v[i] = strtok(NULL, sep);
            if (v[i] == NULL) {
                break;
            }
        }
        /* assert i is number of tokens */
        
        /* Check for background execution (&) */
        background = 0;
        if (i > 1 && strcmp(v[i - 1], "&") == 0) {
            background = 1;
            v[i - 1] = NULL;	/* Remove & from arguments */
            i--;
            
            /* Remove & from cmd_copy for display */
            len = strlen(cmd_copy);
            if (len > 0) {
                char *amp = strrchr(cmd_copy, '&');
                if (amp != NULL) {
                    *amp = '\0';
                    /* Trim trailing spaces */
                    while (amp > cmd_copy && *(amp - 1) == ' ') {
                        *(--amp) = '\0';
                    }
                }
            }
        }
        
        /* Check if command is empty after parsing */
        if (v[0] == NULL) {
            continue;
        }
        
        /* Handle built-in cd command */
        if (strcmp(v[0], "cd") == 0) {
            if (v[1] == NULL) {
                /* No argument, go to home directory */
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
            continue;	/* Don't fork for cd */
        }
        
        /* fork a child process to exec the command in v[0] */
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
                    add_job(frkRtnVal, cmd_copy);
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
