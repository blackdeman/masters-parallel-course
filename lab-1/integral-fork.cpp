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

    const size_t p = (size_t) atoi(argv[1]);
    const size_t m = 1000;

    const double step = (b - a) / m;

    const size_t shmsize = p * sizeof(double);

    int processId = 0;
    double left, right;

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
    for (size_t i = 0; i < p; ++i) {
        processId = i + 1;
        left = a + i * m / p * step;
        right = a + (i + 1) * m / p * step < b ? a + (i + 1) * m / p * step : b;

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
        printf("Child id = %d started calculation from %.3f to %.3f\n", processId, left, right);

        // attach shared memory
        if ((shm = shmat(shmid, NULL, 0)) == (void *) -1) {
            perror("shmat");
            return 1;
        }

        // child calculation
	// TODO change calculation method 
        double sum = 0.;
        for (double i = left; i < right; i += step) {
            sum += step * func(i + step / 2);
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
