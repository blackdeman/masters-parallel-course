#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static volatile int stopping = 0;

void sig_handler(int sig) {
    if (sig == SIGINT) {
        stopping = 1;
    }
}

int main() {

    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        perror("signal");
        return 1;
    }

    int message_len = 100;

    key_t shm_key = 444, sem_key = 4444;

    int shmid;
    void *shm;

    // get shared memory
    if ((shmid = shmget(shm_key, (size_t) message_len, 0666)) < 0) {
        perror("shmget");
        return 1;
    }

    // attach shared memory
    if ((shm = shmat(shmid, NULL, 0)) == (void *) -1) {
        perror("shmat");
        return 1;
    }

    // get semaphore
    int semid = semget(sem_key, 2, 0666);
    if (semid == -1) {
        perror("semget");
        return 1;
    }

    sembuf operations[2];

    while (!stopping) {
        operations[0].sem_num = 0;
        operations[0].sem_op = 0;
        operations[0].sem_flg = 0;

        operations[1].sem_num = 0;
        operations[1].sem_op = 1;
        operations[1].sem_flg = 0;


        printf("Trying to get semaphore...\n");
        if (semop(semid, operations, 2) == -1) {
            fprintf(stderr, "semop failed\n");
            exit(1);
        }

        printf("Enter message: \n");
        fgets((char *) shm, message_len, stdin);
        ((char *) shm)[strlen((char *) shm) - 1] = '\0';

        operations[0].sem_num = 1;
        operations[0].sem_op = 1;
        operations[0].sem_flg = 0;

        if (semop(semid, operations, 1) == -1) {
            fprintf(stderr, "semop failed\n");
            exit(1);
        }
    }

    if (shmdt(shm) == -1) {
        fprintf(stderr, "shmdt failed\n");
        exit(1);
    }

    return 0;
}
