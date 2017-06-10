#include <stdio.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <sys/sem.h>
#include <fcntl.h>

union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};
struct Queue {
    int numJobs;
    int size;
    int location;
    char *jobs;
};

void SetVariables();

union semun semarg;
struct Queue queue;
char *data;
int semaphoreSemId, readSemId, semaphoreK, writeSemaphoreK;
int writeSemId, readSemaphoreK, outputFile, shmKey, shmid;
int writeSemaphoreDesc, readSemaphoreDesc, semaphoreDesc;
#define SHM_SIZE 4096

int main() {
    int shmid;
    char *data;
    char move;
    union semun arg;
    struct semid_ds buf;
    struct sembuf sb;
    key_t key;
    int file;
    int internal_count = 0;

    queue.location = 0;
    queue.numJobs = 0;
    queue.jobs = (char *) malloc((sizeof(char) * 2));
    if (queue.jobs == NULL) {
        //todo handle
    }
    queue.size = 2;

    SetVariables();
    CreateMemory();
    CreateKeys();
    CreateSemaphors();
    file = open("203405386.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (file < 0) {
        perror("failed open file");
        //todo handle
    }

    /* make the key: */
    if ((key = ftok("203405386.txt", 'k')) == -1) {
        write(STDERR_FILENO, "failed ftok", strlen("failed ftok"));
        exit(1);
    }

}

void CreateSemaphors() {
    semarg.val = 1;
    //try getting the semaphore
    writeSemId = semget(writeSemaphoreK, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (writeSemId < 0) { /* someone else created it first */
        writeSemId = semget(writeSemaphoreK, 1, 0); /* get the id */
        if (writeSemId < 0) {
            perror("failed creating semaphore");
            //todo release and exit
        }
    }
    if ((semctl(writeSemId, 0, SETVAL, semarg)) != 0) {
        perror("failed to set semaphor val");
        //todo handle
    }

    //try getting the semaphore
    readSemId = semget(readSemaphoreK, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (readSemId < 0) { /* someone else created it first */
        readSemId = semget(readSemaphoreK, 1, 0); /* get the id */
        if (readSemId < 0) {
            perror("failed creating semaphore");
            //todo release and exit
        }
    }
    if ((semctl(readSemId, 0, SETVAL, semarg)) != 0) {
        perror("failed to set semaphor val");
        //todo handle
    }

    //try getting the semaphore
    semaphoreSemId = semget(semaphoreK, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (semaphoreSemId < 0) { /* someone else created it first */
        semaphoreSemId = semget(semaphoreK, 1, 0); /* get the id */
        if (semaphoreSemId < 0) {
            perror("failed creating semaphore");
            //todo release and exit
        }
    }
    if ((semctl(semaphoreSemId, 0, SETVAL, semarg)) != 0) {
        perror("failed to set semaphor val");
        //todo handle
    }
}

void CreateKeys() {
    //create new key for the shared memory.
    if ((writeSemaphoreK = ftok("writeSemaphore.c", 'A')) == -1) {
        perror("failed to create key");
        //todo handle
    }

    //create new key for the shared memory.
    if ((readSemaphoreK = ftok("readSemaphore.c", 'B')) == -1) {
        perror("failed to create key");
        //todo handle
    }

    //create new key for the shared memory.
    if ((semaphoreK = ftok("semaphore.c", 'C')) == -1) {
        perror("failed to create key");
        //todo handle
    }
}

void CreateMemory() {
    //create the output file.
    if ((outputFile = open("203405386.txt", O_RDWR | O_CREAT | O_APPEND, 0666)) == -1) {
        perror("failed open file");
        //todo handle
    }

    //create key for memory.
    if ((shmKey = ftok("203405386.txt", 'H')) == -1) {
        perror("failed ftok");
        //todo handle
    }
    //create  shared memory.
    if ((shmid = shmget(shmKey, SHM_SIZE, 0644 | IPC_CREAT)) == -1) {
        perror("failed create memory");
        //todo handle
    }
    //attach to  memroy.
    data = shmat(shmid, NULL, 0644);
    if ((*data) == (char *) (-1)) {
        perror("failed attached to memory");
        //todo handle
    }

}

void SetVariables() {
    //create  semaphore key.
    if ((writeSemaphoreDesc = open("writeSemaphore.c", O_RDWR | O_CREAT, 0666)) == -1) {
        perror("failed open file");
        //todo handle
    }

    //create sec semaphore key.
    if ((readSemaphoreDesc = open("readSemaphore.c", O_RDWR | O_CREAT, 0666)) == -1) {
        perror("failed open file");
        //todo handle
    }

    //create third semaphore.
    if ((semaphoreDesc = open("semaphore.c", O_RDWR | O_CREAT, 0666)) == -1) {
        perror("failed open file");
        //todo handle
    }

}