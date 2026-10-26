#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

static volatile sig_atomic_t keep_running = 1;
static volatile sig_atomic_t timed_out = 0;

void end_game(int score) {
    printf("Your score is %d: ", score);
}

void handle_signal(int signum) {
    if (signum == SIGALRM) {
        timed_out = 1;
    }
    keep_running = 0;
}

int main() {

    int a, b, product, score = 0;
    struct sigaction action;

    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGALRM, &action, NULL);

    srand(time(NULL));

    while (keep_running) {
        int scan_result, ch;

        a = rand() % 15;
        b = rand() % 15;

        printf("The product of %d and %d is: ", a , b);
        fflush(stdout);

        alarm(5);
        scan_result = scanf("%d", &product);
        alarm(0);

        if (!keep_running) {
            break;
        }

        if (scan_result == EOF) {
            printf("\nEnd of input detected. Exiting game!\n");
            break;
        }

        if (scan_result == 1 && (a * b) == product) {
            score++;
        } else if (scan_result == 1) {
            printf("Wrong! No score is given\n");            
        }

        while ((ch = getchar()) != '\n' && ch != EOF)
            ;

    }

    if (timed_out) {
        printf("\nTIME's up!\n");
    }

    printf("\nYour final score is: %d\n", score);

    return 0;
}
