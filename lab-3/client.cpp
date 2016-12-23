#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {

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

    struct sembuf sops[2];

    while (1) {
	// check that message is empty
        sops[0].sem_num = 0;
        sops[0].sem_op = 0;
        sops[0].sem_flg = 0;
	// and get semaphore
        sops[1].sem_num = 0;
        sops[1].sem_op = 1;
        sops[1].sem_flg = 0;


        printf("Trying to get semaphore...\n");
        if (semop(semid, sops, 2) == -1) {
            fprintf(stderr, "semop failed\n");
            exit(1);
        }

        printf("Enter message: \n");
        fgets((char *) shm, message_len, stdin);
        ((char *) shm)[strlen((char *) shm) - 1] = '\0';

	// signal to server that message was written
        sops[0].sem_num = 1;
        sops[0].sem_op = 1;
        sops[0].sem_flg = 0;

        if (semop(semid, sops, 1) == -1) {
            fprintf(stderr, "semop failed\n");
            exit(1);
        }
    }

    // detach shared memory
    if (shmdt(shm) == -1) {
        fprintf(stderr, "shmdt failed\n");
        exit(1);
    }

    return 0;
}
