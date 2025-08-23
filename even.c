#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
void handle_int(int signum) {
    printf("Yeah!\n");
}
void handle_hup(int signum) {
    printf("Ouch!\n");
}
int main(int argc,char *argv[]) {
    if (argc!= 2) {
        fprintf(stderr,"Usage: %s <n>\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);
    if (n < 0) {
        fprintf(stderr, "Please provide a non-negative integer for n.\n");
        return 1;
    }
    signal(SIGHUP, handle_hup);
    signal(SIGINT, handle_int);

    for (int i=0;i<n;++i) {
        printf("%d\n",2*i);
        sleep(5);
    }

    return 0;
}