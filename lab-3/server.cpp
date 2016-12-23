#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>

int main() {

    size_t message_len = 100;

    key_t shm_key = 444, sem_key = 4444;

    int shmid;
    void *shm;

    // create shared memory
    if ((shmid = shmget(shm_key, message_len, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        return 1;
    }

    // attach shared memory
    if ((shm = shmat(shmid, NULL, 0)) == (void *) -1) {
        perror("shmat");
        return 1;
    }

    // create semaphore
    int semid = semget(sem_key, 2, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("semget");
        return 1;
    }

    int initvals[2] = {0, 0};
    // set semaphore values
    if (semctl(semid, 0, SETALL, initvals) == -1) {
        perror("semctl setval");
        exit(1);
    }

    struct sembuf sops[1];

    while (1) {
	// waiting for some client to write message
        sops[0].sem_num = 1;
        sops[0].sem_op = -1;
        sops[0].sem_flg = 0;

        printf("Waiting for messages...\n");
        if (semop(semid, sops, 1) == -1) {
            perror("semop 1");
            return 1;
        }

        printf("Recieved:\n%s\n", (char *) shm);

	// message was read, allow clients to get semaphore
        sops[0].sem_num = 0;
        sops[0].sem_op = -1;
        sops[0].sem_flg = 0;

        if (semop(semid, sops, 1) == -1) {
            perror("semop 2");
            return 1;
        }

    }


    // close semaphore
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl");
        return 1;
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

    return 0;
}
