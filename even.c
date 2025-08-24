#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

/*handle int function*/
void handle_int(int signum) {
    printf("Yeah!\n");
}
/*handle hup function*/
void handle_hup(int signum) {
    printf("Ouch!\n");
}

int main(int argc,char *argv[]) {
    /*check if exactly one argument is provided*/
    if (argc!= 2) {
        fprintf(stderr,"Usage: %s <n>\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);
    /*give a warning when users type negative integer*/
    if (n < 0) {
        fprintf(stderr, "Please provide a non-negative integer for n.\n");
        return 1;
    }
    /*register signal handlers*/
    signal(SIGHUP, handle_hup);
    signal(SIGINT, handle_int);
    /*print first n even numbers with 5-second pauses*/
    for (int i=0;i<n;++i) {
        printf("%d\n",2*i);
        sleep(5);
    }

    return 0;
}