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
#include <time.h>

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


/* Thread */
struct thread {
    int id;                              /* friendly id               */
    pthread_t pthread;                   /* pointer to actual thread  */
    struct thpool_ *thpool_p;            /* access to thpool          */
};


/* Threadpool */
struct thpool_ {
    struct thread **threads;                  /* pointer to threads        */
    volatile int num_threads_alive;      /* threads currently alive   */
    volatile int num_threads_working;    /* threads currently working */
    pthread_mutex_t thcount_lock;       /* used for thread count etc */
    struct Queue queue;
};

void SetVariables();

void CreateSemaphors();

void CreateMemory();

void CreateKeys();

int Keep_on = 1;
union semun semarg;
struct Queue queue;
char *data;
int semaphoreSemId, readSemId, queueSemaphoreK, writeSemaphoreK;
int writeSemId, readSemaphoreK,outputSemaphoreK, outputFile, shmKey, shmid;
int writeSemaphoreDesc, readSemaphoreDesc, semaphoreDesc;
int internal_count;
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
    internal_count = 0;
    char job;
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

    while (Keep_on) {


        //read char from memo

        SearchForThread(job);
    }
}

void SearchForThread(char job) {
    pthread_t threadPid;
    int flag;
    //wait for thread

    //do the job
    ExecJob(job, threadPid);
}

int ExecJob(char job, pthread_t tid) {
    srand(time(NULL));
    int writen;
    char finalDetails[1028];
    char pidDescription[512];
    char inCounterDescription[128];
    int randNum = (rand() % 90) + 10;
    int numToAddToInternal = 0;
    if (((job >= 97) && (job <= 101)) || ((job >= 65) && (job <= 69))) {
        struct timespec time;
        time.tv_sec = 0;
        time.tv_nsec = randNum;
        nanosleep(&time, NULL);
    }
    switch (job) {
        case 'a':
        case 'A':
            numToAddToInternal = 1;
            break;
        case 'b':
        case 'B':
            numToAddToInternal = 2;
            break;
        case 'c':
        case 'C':
            numToAddToInternal = 3;
            break;
        case 'd':
        case 'D':
            numToAddToInternal = 4;
            break;
        case 'e':
        case 'E':
            numToAddToInternal = 5;
            break;
        case 'f':
        case 'F':
            break;
        case 'g':
        case 'G':
            EndGame();
            break;
        case 'h':
        case 'H':
            WaitAndEndGame();
        default:
            break;
    }
    //todo close sem
    internal_count += numToAddToInternal;

    memset(finalDetails, 0, 1028);
    memset(pidDescription, 0, 512);
    memset(inCounterDescription, 0, 128);

    writen = snprintf(inCounterDescription, 128, "%d", numToAddToInternal);
    if (writen < 0) {
        perror("failed converting grade to string");
        //todo handle
    }
    writen = snprintf(pidDescription, 512, "%ld", tid);
    if (writen < 0) {
        perror("failed converting grade to string");
        //todo handle
    }

    //create one long string
    memset(finalDetails, 0, 1028);
    strcpy(finalDetails, "thread identifier is ");
    strcat(finalDetails, pidDescription);
    strcat(finalDetails, " ");
    strcat(finalDetails, "and internal_count is ");
    strcat(finalDetails, inCounterDescription);
    strcat(finalDetails, "\n");

    //write this string
    writen = write(outputFile, finalDetails, strlen(finalDetails));
    if (writen < 0) {
        perror("failed to write to file");
        //todo handle
    }

    //todo release semaphore
}

void EndGame() {

}

void WaitAndEndGame() {

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
    if ((shmKey = ftok("203405386.txt", 'K')) == -1) {
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