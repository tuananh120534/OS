/*********************************************************************
   Program  : miniShell                   Version    : 1.3
 --------------------------------------------------------------------
   skeleton code for linix/unix/minix command line interpreter
 --------------------------------------------------------------------
   File			: minishell.c
   Compiler/System	: gcc/linux

********************************************************************/

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#define NV 20			/* max number of command tokens */
#define NL 100			/* input buffer size */
#define MAX_BG_JOBS 50		/* max number of background jobs */

char            line[NL];	/* command input buffer */

/* Structure to track background jobs */
struct bg_job {
    pid_t pid;
    char *command;
    int active;
} bg_jobs[MAX_BG_JOBS];

int bg_job_count = 0;

/*
	shell prompt
 */
void prompt(void)
{
  fprintf(stdout, "\n msh> ");
  if (fflush(stdout) == EOF) {
    perror("fflush");
  }
}

/* Check for completed background jobs */
void check_background_jobs(void)
{
    int status;
    pid_t pid;
    int i;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (i = 0; i < bg_job_count; i++) {
            if (bg_jobs[i].active && bg_jobs[i].pid == pid) {
                printf("[%d]+ Done                 %s\n", i + 1, bg_jobs[i].command);
                if (fflush(stdout) == EOF) {
                    perror("fflush");
                }
                bg_jobs[i].active = 0;
                free(bg_jobs[i].command);
                bg_jobs[i].command = NULL;
                break;
            }
        }
    }
}

/* Add a background job to tracking */
void add_background_job(pid_t pid, char *command)
{
    if (bg_job_count < MAX_BG_JOBS) {
        bg_jobs[bg_job_count].pid = pid;
        bg_jobs[bg_job_count].command = strdup(command);
        if (bg_jobs[bg_job_count].command == NULL) {
            perror("strdup");
            return;
        }
        bg_jobs[bg_job_count].active = 1;
        printf("[%d] %d\n", bg_job_count + 1, (int)pid);
        if (fflush(stdout) == EOF) {
            perror("fflush");
        }
        bg_job_count++;
    }
}

/* Handle built-in cd command */
int handle_builtin_cd(char *v[])
{
    if (v[0] != NULL && strcmp(v[0], "cd") == 0) {
        if (v[1] == NULL || strlen(v[1]) == 0) {
            /* cd with no arguments - go to home directory */
            char *home = getenv("HOME");
            if (home == NULL) {
                fprintf(stderr, "cd: HOME not set\n");
                return 1;
            }
            if (chdir(home) == -1) {
                perror("chdir");
            }
        } else {
            /* cd with directory argument */
            if (chdir(v[1]) == -1) {
                perror("chdir");
            }
        }
        return 1; /* Built-in command handled */
    }
    return 0; /* Not a built-in command */
}

/* argk - number of arguments */
/* argv - argument vector from command line */
/* envp - environment pointer */
int main(int argk, char *argv[], char *envp[])
{
    int             frkRtnVal;	    /* value returned by fork sys call */
    char           *v[NV];	        /* array of pointers to command line tokens */
    char           *sep = " \t\n";  /* command line token separators    */
    int             i;		          /* parse index */
    int             background;     /* flag for background execution */
    char            original_line[NL]; /* store original command for background jobs */

    /* prompt for and process one command line at a time  */
    while (1) {			/* do Forever */
        /* Check for completed background jobs before each prompt */
        check_background_jobs();
        
        prompt();
        if (fgets(line, NL, stdin) == NULL) {
            if (feof(stdin)) {
                exit(0);
            }
            perror("fgets");
            continue;
        }

        /* This if() required for gradescope */
        if (feof(stdin)) {		/* non-zero on EOF  */
            exit(0);
        }
        
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\000'){
            continue;			/* to prompt */
        }

        /* Store original line for background job tracking */
        strcpy(original_line, line);
        
        /* Parse command line */
        v[0] = strtok(line, sep);
        if (v[0] == NULL) {
            continue;
        }
        
        for (i = 1; i < NV; i++) {
            v[i] = strtok(NULL, sep);
            if (v[i] == NULL){
                break;
            }
        }
        /* assert i is number of tokens + 1 */

        /* Handle built-in commands first, before checking for background */
        if (handle_builtin_cd(v)) {
            continue; /* Built-in command was handled */
        }

        /* Check for background execution (&) */
        background = 0;
        if (i > 0) {
            /* Check if last non-null argument is & */
            int last_arg = i - 1;
            if (v[last_arg] != NULL && strcmp(v[last_arg], "&") == 0) {
                background = 1;
                v[last_arg] = NULL; /* Remove & from arguments */
                i--; /* Adjust token count */
            }
        }

        /* fork a child process to exec the command in v[0] */
        switch (frkRtnVal = fork()) {
            case -1:			/* fork returns error to parent process */
            {
                perror("fork");
                break;
            }
            case 0:			/* code executed only by child process */
            {
                if (execvp(v[0], v) == -1) {
                    perror("execvp");
                    exit(1); /* Terminate child process on exec failure */
                }
                break; /* This should never be reached */
            }
            default:			/* code executed only by parent process */
            {
                if (background) {
                    /* Background process - don't wait, just track it */
                    /* Create clean command string for display */
                    char clean_cmd[NL];
                    int j, pos = 0;
                    
                    for (j = 0; j < i && v[j] != NULL; j++) {
                        if (j > 0) {
                            clean_cmd[pos++] = ' ';
                        }
                        strcpy(clean_cmd + pos, v[j]);
                        pos += strlen(v[j]);
                    }
                    clean_cmd[pos] = '\0';
                    
                    add_background_job(frkRtnVal, clean_cmd);
                } else {
                    /* Foreground process - wait for completion */
                    if (wait(0) == -1) {
                        perror("wait");
                    }
                }
                break;
            }
        }				/* switch */
    }				/* while */
}				/* main */