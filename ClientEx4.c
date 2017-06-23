//
// Created by haim on 10/06/17.
//

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <sys/sem.h>

union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};
union semun semarg;
#define SHM_SIZE 4096
char *memory;
int writeId,thirdMutexId,readId;
key_t  memKey,thirdKey;

void EndProgram();

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

int main() {

    key_t key;
    key_t writerKey;
    key_t readerKey;
    int shmid;
    char *data;
    char move;
    union semun arg;
    struct semid_ds buf;
    struct sembuf sb;
    int semid;

    /*write semaphore*/

    //create new key for the first semaphore.
    if ((writerKey = ftok("writeSemaphore.c", 'A')) == -1) {
        perror("failed ftok");
        exit(1);
    }

    //try getting the semaphore
    writeId = semget(writerKey, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (writeId < 0) { /* someone else created it first */
        writeId = semget(writerKey, 1, 0); /* get the id */
        if (writeId < 0) {
            perror("failed creating semaphore");
            exit(0);
        }
    }

    /*read semaphore*/

    if ((readerKey = ftok("readSemaphore.c", 'B')) == -1) {
        write(STDERR_FILENO, "failed ftok", strlen("failed ftok"));
        exit(1);
    }
    //try getting the semaphore
    readId = semget(readerKey, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (readId < 0) { /* someone else created it first */
        readId = semget(readerKey, 1, 0); /* get the id */
        if (readId < 0) {
            perror("failed creating semaphore");
            exit(0);
        }
    }

    /*third(inner) semaphore*/

    if ((thirdKey = ftok("semaphore.c", 'C')) == -1) {
        perror("failed creating semaphore");
        exit(0);
    }
    //try getting the semaphore
    thirdMutexId = semget(thirdKey, 1, IPC_CREAT | IPC_EXCL | 0666);
    if ( thirdMutexId  < 0) { /* someone else created it first */
        thirdMutexId = semget(thirdKey, 1, 0); /* get the id */
        if (thirdMutexId < 0) {
            perror("failed creating semaphore");
            exit(0);
        }
    }

    /* the memoty key */
    if ((memKey = ftok("203405386.txt", 'K')) == -1) {
        perror("failed ftok");
        exit(1);
    }

    /* connect to segment */
    if ((shmid = shmget(memKey, SHM_SIZE, 0644 | IPC_CREAT)) == -1) {
        perror("shmget");
        exit(1);
    }

    /* attach to segment */
    data = shmat(shmid, NULL, 0);
    memory = data;
    if (data == (char *) (-1)) {
        perror("shmat");
        exit(1);
    }


    do {
        /*ask user for input*/
        if (write(STDOUT_FILENO, "Please enter request code\n", strlen("Please enter request code\n")) < 0) {
            perror("failed to write to screen");
            exit(0);
        }

        /*if (read(STDIN_FILENO, &move, 1) < 0) {
            perror("failed to read from stdin");
            //todo ReleaseMemoryEndExit();
        }*/
        char dummy;
        scanf("%c%c",&move,&dummy);
        if (move == 'i') {
            EndProgram();
        }else{

            sb.sem_num = 0;
            sb.sem_op = -1;
            sb.sem_flg = SEM_UNDO;

            //try close(=wait) writer
            if(semop(writeId, &sb, 1)<0){
                perror("faild semop");
                EndProgram();
            }
            //close third(inner) sem
            if(semop(thirdMutexId,&sb,1)<0){
                perror("faild semop");
                EndProgram();
            }

            *data = move;
            //add item

            sb.sem_num = 0;
            sb.sem_op = 1;
            sb.sem_flg = SEM_UNDO;

            //release third sem
            if(semop(thirdMutexId,&sb,1)<0){
                perror("faild semop");
                EndProgram();
            }

            // release reader
            if(semop(readId, &sb, 1)<0){
                perror("faild semop");
                EndProgram();
            }

        }
    } while (1);
    return 0;
}

#pragma clang diagnostic pop

void EndProgram() {

    if(shmdt(memory)<0){
        perror("failed detach memory");
    }

    exit(0);
}