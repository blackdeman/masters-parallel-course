#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>

double func(double x) {
    return x * x * x + 3;
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

    const size_t shmsize = p * sizeof(double);

    int processId = 0;
    double left;
    int steps;

    key_t key = 2468;
    pid_t pid;

    int shmid;
    void *shm;

    // create shared memory
    if ((shmid = shmget(key, shmsize, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        return 1;
    }

    pid_t* children = (pid_t *) malloc(sizeof(pid_t) * p);

    // fork chuldren and collect ids for waiting
    for (int i = 0; i < p; ++i) {
        processId = i + 1;
        left = a + i * m / p * step;
        steps = m / p;

        if ((pid = fork()) == -1) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {
            break;
        }
        children[i] = pid;
    }


    if (pid == 0) {
        // child process
        printf("Child id = %d started calculation start %.3f, count %d\n", processId, left, steps);

        // attach shared memory
        if ((shm = shmat(shmid, NULL, 0)) == (void *) -1) {
            perror("shmat");
            return 1;
        }

        // child calculation
        double sum = 0.;
        // left rectangle method
        for (double i = 0; i < steps; ++i) {
            sum += step * func(left + i * step);
        }
        ((double*)shm)[processId - 1] = sum;

        // detach shared memory
        if (shmdt(shm) == -1) {
            perror("shmdt");
            return 1;
        }
    } else {
        // parent process

        // attach shared memory
        if ((shm = shmat(shmid, NULL, 0)) == (void *) -1) {
            perror("shmat");
            return 1;
        }

        // wait for children to finish
        int exitCode = 0;
        int status;
        for (size_t i = 0; i < p; ++i) {
            if (waitpid(children[i], &status, 0) == -1) {
                perror("waitpid");
                return 1;
            }
            if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) != 0) {
                    exitCode = 1;
                }
            }
        }

        if (exitCode) {
            fprintf(stderr, "Some of child processes failed");
        } else {
            // collect results
            double result = 0.;
            for (size_t i = 0; i < p; ++i) {
                result += ((double*)shm)[i];
            }
            printf("Result : %.5f\n", result);
        }

        // detach shared memory
        if (shmdt(shm) == -1) {
            perror("shmdt");
            return 1;
        }

        // remove shared memory
        if (shmctl(shmid, IPC_RMID, NULL)) {
            perror("shmctl");
            return 1;
        }

        free(children);

        return exitCode;
    }

    return 0;
}