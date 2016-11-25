#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

double result;

pthread_mutex_t mutexsum;

double func(double x) {
    return x * x * x + 3;
}

struct threadArgs {
    int id;
    double left;
    int steps;
    double step;
};

void *PrintHello(void *arguments) {
    struct threadArgs *args = (struct threadArgs *)arguments;

    printf("Child id = %d started calculation start %.3f, count %d\n", args->id, args->left, args->steps);

    double sum = 0.;
    for (double i = 0; i < args->steps; ++i) {
        sum += args->step * func(args->left + i * args->step);
    }

    pthread_mutex_lock (&mutexsum);
    result += sum;
    pthread_mutex_unlock (&mutexsum);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Argument for process count is required!");
        return 1;
    }

    const double a = 0;
    const double b = 1;

    const int p = atoi(argv[1]);
    const int m = 10000;

    const double step = (b - a) / m;
    double left, right;

    result = 0.0;

    pthread_t *threads = (pthread_t *) malloc(p * sizeof(pthread_t));

    pthread_mutex_init(&mutexsum, NULL);

    int rc;
    long threadId;
    for (int i = 0; i < p; i++) {

        struct threadArgs *curArgs = (struct threadArgs *)malloc(sizeof(struct threadArgs));
        curArgs->id = i + 1;
        curArgs->left = a + i * m / p * step;
        curArgs->steps = m / p;
        curArgs->step = step;

        rc = pthread_create(&threads[i], NULL, PrintHello, (void *) curArgs);
        if (rc) {
            fprintf(stderr, "ERROR; return code from pthread_create() is %d\n", rc);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < p; ++i) {
        rc = pthread_join(threads[i], NULL);
        if (rc) {
            fprintf(stderr, "ERROR: return code from pthread_join() is %d\n", rc);
            exit(EXIT_FAILURE);
        }
    }

    printf("Result : %.5f\n", result);

    pthread_mutex_destroy(&mutexsum);
    free(threads);
    pthread_exit(NULL);
}