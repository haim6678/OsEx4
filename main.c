/******************************************
*
Student name: Haim Rubinstein
*
Student ID: 203405386
*
Course Exercise Group:01
*
Exercise name:Ex42
******************************************/

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
#include <pthread.h>


//declare all the vars
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


// Thread struct
struct thread {
    int id;
    pthread_t pthreadId;
    struct thpool_ *thpool_p;
};


/* Threadpool */
struct thpool {
    struct thread threads[5];
    volatile int num_threads_alive;
    volatile int num_threads_working;
    pthread_mutex_t queueAccessLock;
    pthread_mutexattr_t queueLOckAttr;
    pthread_mutex_t counterLock;
    pthread_mutexattr_t counterLockAttr;
    struct Queue queue;
};

void SetVariables();

char NextJob();

void *ThreadFunc();

void CreateSemaphors();

void CounterLockSet();

void ExecJob(char job, pthread_t tid);

void SetSems();

void CreateMemory();

void CreatePool();

void CreateKeys();

void EndGame();

void ListenTOjobs();

void WaitAndEndGame();

void WriteToResultFile(int flag);

void ClearResources();

int numThredFinishe = 0;
int Keep_on = 1;
union semun semarg;
struct sembuf sb;
struct Queue queue;
struct thpool threadPool;
char *data;
pthread_mutex_t countFinishedLock, fileLock;
pthread_mutexattr_t countFinishedAttr, fileLockAttr;
int semaphoreSemId, readSemId, queueSemaphoreK, writeSemaphoreK;
int writeSemId, readSemaphoreK, outputSemaphoreK, outputFile, shmKey, shmid;
int writeSemaphoreDesc, readSemaphoreDesc, semaphoreDesc, semaphoreK;
int internal_count;
#define SHM_SIZE 4096

/**
 * the operation - runs the program
 */
int main() {
    //declare variables

    internal_count = 0;
    queue.location = 0;
    queue.numJobs = 0;


    CounterLockSet();
    //make place for jobs
    queue.jobs = (char *) malloc((sizeof(char) * 2));
    if (queue.jobs == NULL) {
        perror("failed to allocat place");
        exit(0);
    }
    queue.size = 2;

    SetSems();
    CreateMemory();

    //get the jobs from shared memory
    ListenTOjobs();

    return 0;
}

/**
 * the operation - define the num thread counter mutex
 */
void CounterLockSet() {
    //the queue lock
    if ((pthread_mutexattr_init(&(countFinishedAttr)) < 0)) {
        perror("failed init mutex attr");
        exit(1);
    }

    //set the mutex attr.
    if ((pthread_mutexattr_settype(&(countFinishedAttr), PTHREAD_MUTEX_ERRORCHECK)) < 0) {
        perror("failed setting mutex attr");
        exit(1);
    }
    //init the mutex
    if (pthread_mutex_init((&countFinishedLock), (&(threadPool.queueLOckAttr))) != 0) {
        perror("faile creating mutex");
        exit(1);
    }
}

/**
 * the input - a char that represent the job
 * the operation - after jetting a job from memory
 *                  enter the job to the queue
 */
void EnterJobToQueue(char job) {

    int tepmSize;
    //lock the access for one thread
    if (pthread_mutex_lock(&(threadPool.queueAccessLock)) != 0) {
        perror("failed locking mutex");
    }
    //if need inlarge the quque
    if (queue.size == (queue.location - 1)) {
        tepmSize = queue.size;
        tepmSize *= 2;
        queue.jobs = realloc(queue.jobs, sizeof(char) * tepmSize);
        queue.size = tepmSize;
    }

    //enter to queue
    queue.jobs[queue.location] = job;
    queue.numJobs++;
    //release the access
    if (pthread_mutex_unlock(&(threadPool.queueAccessLock)) != 0) {
        perror("failed locking mutex");
    }
}

/**
 * the operation - wait until we have a message to read
 *                  then read it and input to queue
 */
void ListenTOjobs() {
    char job;

    while (1) {
        //get the jobs from memory
        //init vals for action
        sb.sem_num = 0;
        sb.sem_op = -1;
        sb.sem_flg = SEM_UNDO;
        //close the full(=reader) semaphore until there is data inside
        if (semop(readSemId, &sb, 1) < 0) {
            perror("faild semop");
            ClearResources();
            exit(0);
        }
        //now there is data inside
        //close inner semaphore and prevent from other to read or write
        if (semop(semaphoreSemId, &sb, 1) < 0) {
            perror("faild semopp");
            ClearResources();
            exit(0);
        }

        //get the job
        job = *data;
        semarg.val = 0;
        if (semctl(readSemId, 0, SETVAL, semarg) < 0) {
            perror("failed to set semaphore val");
            ClearResources();
            exit(0);
        }

        //init vals for release
        sb.sem_num = 0;
        sb.sem_op = 1;
        sb.sem_flg = SEM_UNDO;
        //release inner

        if ((job == 'g') || (job == 'G')) {
            EndGame();
        } else if ((job == 'h') || (job == 'H')) {
            WaitAndEndGame();
        } else {
            EnterJobToQueue(job);
            if (semop(semaphoreSemId, &sb, 1) < 0) {
                perror("faild semop");
                ClearResources();
                exit(0);
            }
            //release writer semaphore in the other proccess
            if (semop(writeSemId, &sb, 1) < 0) {
                perror("faild semop");
                ClearResources();
                exit(0);
            }
        }

    }
}


/**
 * the operation - sets all the semaphore and mutex
 */
void SetSems() {

    //init all needed fields
    SetVariables();
    CreateKeys();
    CreateSemaphors();
    CreatePool();
}

/**
 * the operation - the function that the thread will get when he starts
 */
void *ThreadFunc() {
    char job = '#';
    while (job != '$') {
        //get job from queue
        job = NextJob();

        if ((job != '#') && (job != '$')) {
            //exec it
            ExecJob(job, pthread_self());
        }
    }

    if (pthread_mutex_lock(&(countFinishedLock)) != 0) {
        perror("failed locking mutex");
    }
    numThredFinishe++;
    if (pthread_mutex_unlock(&(countFinishedLock)) != 0) {
        perror("failed locking mutex");
    }
    WriteToResultFile(1);
    pthread_exit((void *) 0);
}

/**
 * the output - the next job in queue or # if there isn't one
 * the operation- gets the job from queue
 */
char NextJob() {
    char temp = '#';

    //lock the access to queue
    if (pthread_mutex_lock(&(threadPool.queueAccessLock)) != 0) {
        perror("failed locking mutex");
        ClearResources();
        exit(0);
    }
    //get the job
    if (queue.numJobs > 0) {
        temp = queue.jobs[queue.location];
        if (temp != '$') {
            queue.location++;
            queue.numJobs--;
        }
    }
    //release the lock
    if (pthread_mutex_unlock(&(threadPool.queueAccessLock)) != 0) {
        perror("failed unlocking mutex");
    }
    return temp;
}

/**
 * the input - the job to execute and the tid number
 * the operation - execute the curr job
 */
void ExecJob(char job, pthread_t tid) {
    //init values
    srand(time(NULL));
    int randNum = (rand() % 90) + 10;
    int numToAddToInternal = -1;
    if (((job >= 97) && (job <= 101)) || ((job >= 65) && (job <= 69))) {
        struct timespec time;
        time.tv_sec = 0;
        time.tv_nsec = randNum;
        nanosleep(&time, NULL);
    }
    //check what is the gob the execute
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
            WriteToResultFile(1);
            break;
        default:
            break;
    }

    if (numToAddToInternal != -1) {
        //lock access to number
        if (pthread_mutex_lock(&(threadPool.counterLock)) != 0) {
            perror("failed locking mutex");
        }

        internal_count += numToAddToInternal;

        if (pthread_mutex_unlock(&(threadPool.counterLock)) != 0) {
            perror("failed locking mutex");
        }
    }
}

/**
 * the operation - write the current internal_count to the file
 */
void WriteToResultFile(int flag) {

    //init buffers
    int writen;
    pthread_t tid = pthread_self();
    char finalDetails[1028];
    char pidDescription[512];
    char inCounterDescription[128];
    memset(finalDetails, 0, 1028);
    memset(pidDescription, 0, 512);
    memset(inCounterDescription, 0, 128);

    //lock access to number
    if (pthread_mutex_lock(&(threadPool.counterLock)) != 0) {
        perror("failed locking mutex");
    }
    //write internal count to buffer
    writen = snprintf(inCounterDescription, 128, "%d", internal_count);
    if (writen < 0) {
        perror("failed converting number to string");
        ClearResources();
        exit(0);
    }
    //lock access to number
    if (pthread_mutex_unlock(&(threadPool.counterLock)) != 0) {
        perror("failed locking mutex");
    }

    //write tid to buffer
    writen = snprintf(pidDescription, 512, "%ld", tid);
    if (writen < 0) {
        perror("failed converting number to string");
        ClearResources();
        exit(0);
    }

    //create one long string
    strcpy(finalDetails, "thread identifier is ");
    strcat(finalDetails, pidDescription);
    strcat(finalDetails, " ");
    strcat(finalDetails, "and internal_count is ");
    strcat(finalDetails, inCounterDescription);
    strcat(finalDetails, "\n");

    if (flag == 1) {
        if (pthread_mutex_lock(&(fileLock)) != 0) {
            perror("failed locking mutex");
        }
    }
    //write this string
    writen = write(outputFile, finalDetails, strlen(finalDetails));
    if (flag == 1) {
        if (pthread_mutex_unlock(&(fileLock)) != 0) {
            perror("failed unlocking mutex");
        }
    }
    if (writen < 0) {
        perror("failed to write to file");
        ClearResources();
        exit(0);
    }
}

/**
 * the operation - clear all the server resources
 */
void ClearResources() {
    /*delete semaphores*/

    //delete write semaphore
    if (semctl(writeSemId, 0, IPC_RMID, 0) < 0) {
        perror("failed to delete semaphore");
        ClearResources();
        exit(0);
    }
    //delete read semaphore
    if (semctl(readSemId, 0, IPC_RMID, 0) < 0) {
        perror("failed to delete semaphore");
        ClearResources();
        exit(0);
    }
    //delete inner semaphore
    if (semctl(semaphoreSemId, 0, IPC_RMID, 0) < 0) {
        perror("failed to delete semaphore");
    }

    /*delete files*/

    //close first file
    if (close(readSemaphoreDesc) < 0) {
        perror("failed close file");
    }
    if (unlink("readSemaphore.c") < 0) {
        perror("failed to delete file");
    }

    //close sec file
    if (close(writeSemaphoreDesc) < 0) {
        perror("failed close file");
    }
    if (unlink("writeSemaphore.c") < 0) {
        perror("failed to delete file");
    }

    //close third file
    if (close(semaphoreDesc) < 0) {
        perror("failed close file");
    }
    if (unlink("semaphore.c") < 0) {
        perror("failed to delete file");
    }

    /* delete semaphores*/

    //first mutex
    if (pthread_mutex_destroy(&(threadPool.counterLock)) < 0) {
        perror("failed destroy mutex");
    }
    //sec mutex
    if (pthread_mutex_destroy(&(threadPool.queueAccessLock)) < 0) {
        perror("failed destroy mutex");
    }
    if (pthread_mutex_destroy(&(fileLock)) < 0) {
        perror("failed destroy mutex");
    }

    if (pthread_mutexattr_destroy(&(threadPool.counterLockAttr)) < 0) {
        perror("failed destroy mutex");
    }
    if (pthread_mutexattr_destroy(&(fileLockAttr)) < 0) {
        perror("failed destroy mutex");
    }
    if (pthread_mutexattr_destroy(&(threadPool.queueLOckAttr)) < 0) {
        perror("failed destroy mutex");
    }

    /* remove memory */
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(1);
    }
}

/**
 * the operation - end the game
 */
void EndGame() {

    //end all threads
    int i = 0;
    for (i; i < 5; i++) {
        if (pthread_cancel(threadPool.threads[i].pthreadId) < 0) {
            perror("failed to cancel thread");
            ClearResources();
            exit(0);
        }
    }

    //write the file the result
    WriteToResultFile(0);
    //clear all resources
    ClearResources();
    exit(0);
}

/**
 * the operation -wait for other threads and then end the game
 */
void WaitAndEndGame() {

    int i = 0;
    for (i; i < 5; i++) {
        EnterJobToQueue('$');
    }
    while (numThredFinishe != 5) {
    }
    WriteToResultFile(1);
    ClearResources();
    exit(0);
}

/**
 * the operation - create the 5 threads
 */
void CreatePool() {

    int i = 0;
    pthread_t tid;
    pthread_mutexattr_t atrr;

    //the file lock
    if ((pthread_mutexattr_init(&(fileLockAttr)) < 0)) {
        perror("failed init mutex attr");
        exit(1);
    }

    //set the mutex attr.
    if ((pthread_mutexattr_settype(&(fileLockAttr), PTHREAD_MUTEX_ERRORCHECK)) < 0) {
        perror("failed setting mutex attr");
        exit(1);
    }
    //init the mutex
    if (pthread_mutex_init((&fileLock), (&(fileLockAttr))) != 0) {
        perror("faile creating mutex");
        exit(1);
    }

    //the queue lock
    if ((pthread_mutexattr_init(&(threadPool.queueLOckAttr)) < 0)) {
        perror("failed init mutex attr");
        exit(1);
    }

    //set the mutex attr.
    if ((pthread_mutexattr_settype(&(threadPool.queueLOckAttr), PTHREAD_MUTEX_ERRORCHECK)) < 0) {
        perror("failed setting mutex attr");
        exit(1);
    }
    //init the mutex
    if (pthread_mutex_init((&threadPool.queueAccessLock), (&(threadPool.queueLOckAttr))) != 0) {
        perror("faile creating mutex");
        exit(1);
    }

    //the counter lock
    if ((pthread_mutexattr_init(&(threadPool.counterLockAttr)) < 0)) {
        perror("failed init mutex attr");
        exit(1);
    }

    //set the mutex attr.
    if ((pthread_mutexattr_settype(&(threadPool.counterLockAttr), PTHREAD_MUTEX_ERRORCHECK)) < 0) {
        perror("failed setting mutex attr");
        exit(1);
    }

    if (pthread_mutex_init((&threadPool.counterLock), (&(threadPool.counterLockAttr))) != 0) {
        perror("faile creating mutex");
        exit(1);
    }


    //run in loop and create them
    for (i = 0; i < 5; i++) {
        if (pthread_create(&tid, NULL, &ThreadFunc, NULL) == -1) {
            perror("failed creating thread");
            exit(1);
        }
        threadPool.threads[i].pthreadId = tid;
    }

}

/**
 * the operation - create all 3 semaphors for the program
 */
void CreateSemaphors() {


    /*the write semaphore*/

    //try getting the semaphore
    writeSemId = semget(writeSemaphoreK, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (writeSemId < 0) { /* someone else created it first */
        writeSemId = semget(writeSemaphoreK, 1, 0); /* get the id */
        int err = semctl(writeSemId, 0, IPC_RMID, 0);/*delete nad recreate*/
        writeSemId = semget(writeSemaphoreK, 1, IPC_CREAT | IPC_EXCL | 0666);
        if (writeSemId < 0) {
            perror("failed creating semaphore");
            //todo release and exit
        }
    }


    /*read semaphore*/

    //try getting the semaphore
    readSemId = semget(readSemaphoreK, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (readSemId < 0) { /* someone else created it first */
        readSemId = semget(readSemaphoreK, 1, 0); /* get the id */
        int err = semctl(readSemId, 0, IPC_RMID, 0); /*delete nad recrate*/
        readSemId = semget(readSemaphoreK, 1, IPC_CREAT | IPC_EXCL | 0666);
        if (readSemId < 0) {
            perror("failed creating semaphore");
            //todo release and exit
        }
    }


    /*third semaphore*/

    //try getting the semaphore
    semaphoreSemId = semget(semaphoreK, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (semaphoreSemId < 0) { /* someone else created it first */
        semaphoreSemId = semget(semaphoreK, 1, 0); /* get the id */
        int err = semctl(semaphoreSemId, 0, IPC_RMID, 0); /*delete nad recrate*/
        semaphoreSemId = semget(semaphoreK, 1, IPC_CREAT | IPC_EXCL | 0666);
        if (semaphoreSemId < 0) {
            perror("failed creating semaphore");
            //todo release and exit
        }
    }

    //set their vals
    semarg.val = 1;
    if (semctl(writeSemId, 0, SETVAL, semarg) < 0) {
        perror("failed to set semaphore val");
        //todo handel
    }

    semarg.val = 1;
    if (semctl(semaphoreSemId, 0, SETVAL, semarg) < 0) {
        perror("failed to set semaphore val");
        //todo handel
    }

    semarg.val = 0;
    if (semctl(readSemId, 0, SETVAL, semarg) < 0) {
        perror("failed to set semaphore val");
        //todo handel
    }

}

/**
 * the operation - creates keys for memory and semaphores
 */
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

/**
 * the operation - create shared memory
 */
void CreateMemory() {
    //create the output file.
    if ((outputFile = open("203405386.txt", O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1) {
        perror("failed open file");
        exit(0);
    }

    //create key for memory.
    if ((shmKey = ftok("203405386.txt", 'K')) == -1) {
        perror("failed ftok");
        exit(0);
    }
    //create  shared memory.
    if ((shmid = shmget(shmKey, SHM_SIZE, 0644 | IPC_CREAT)) == -1) {
        perror("failed create memory");
        exit(0);
    }
    //attach to  memroy.
    data = shmat(shmid, NULL, 0644);
    if (data == (char *) (-1)) {
        perror("failed attached to memory");
        exit(0);
    }

}

/**
 * the operation - open relevat files
 */
void SetVariables() {
    //create  semaphore key.
    if ((writeSemaphoreDesc = open("writeSemaphore.c", O_RDWR | O_CREAT, 0666)) == -1) {
        perror("failed open file");
        exit(0);
    }

    //create sec semaphore key.
    if ((readSemaphoreDesc = open("readSemaphore.c", O_RDWR | O_CREAT, 0666)) == -1) {
        perror("failed open file");
        exit(0);
    }

    //create third semaphore.
    if ((semaphoreDesc = open("semaphore.c", O_RDWR | O_CREAT, 0666)) == -1) {
        perror("failed open file");
        exit(0);
    }

}