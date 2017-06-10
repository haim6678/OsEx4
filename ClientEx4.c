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
    //create new key for the first semaphore.
    if ((writerKey = ftok("writerSem.c", 'A')) == -1) {
        write(STDERR_FILENO, "failed ftok", strlen("failed ftok"));
        exit(1);
    }

    //try getting the semaphore
    semid = semget(writerKey, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (semid < 0) { /* someone else created it first */
        semid = semget(writerKey, 1, 0); /* get the id */
        if (semid < 0) {
            perror("failed creating semaphore");
            //todo release and exit
        }
    }

    if ((writerKey = ftok("writerSem.c", 'A')) == -1) {
        write(STDERR_FILENO, "failed ftok", strlen("failed ftok"));
        exit(1);
    }
    /* make the key: */
    if ((key = ftok("203405386.txt", 'k')) == -1) {
        write(STDERR_FILENO, "failed ftok", strlen("failed ftok"));
        exit(1);
    }

    /* connect to (and possibly create) the segment: */
    if ((shmid = shmget(key, SHM_SIZE, 0644 | IPC_CREAT)) == -1) {
        perror("shmget");
        exit(1);
    }

    /* attach to the segment to get a pointer to it: */
    data = shmat(shmid, NULL, 0);
    memory = data;
    if (data == (char *) (-1)) {
        perror("shmat");
        exit(1);
    }

    //try getting the semaphore
    semid = semget(writerKey, 1, IPC_CREAT | IPC_EXCL | 0666);

    if (semid < 0) { /* someone else created it first */
        semid = semget(writerKey, 1, 0); /* get the id */
        if (semid < 0) {
            perror("failed creating semaphore");
            //todo release and exit
        }
    }


    if (write(STDOUT_FILENO, "Please enter request code\n", strlen("Please enter request code\n")) < 0) {
        perror("failed to write to screen");
        //todo release and exit
    }
    do {
        if (read(STDIN_FILENO, &move, 1) < 0) {
            perror("failed to read from stdin");
            //todo ReleaseMemoryEndExit();
        }
        if (move == 'i') {
            EndProgram();
        }else{
            sb.sem_op = -1;
            sb.sem_flg = SEM_UNDO;
            if (semop(semid, &sb, 1) == -1) {
                perror("failed acting semaphore action");
                //todo ReleaseMemoryEndExit();
            }
            //close writer
            //closer reader

            //relese reader
            //release writer in server!
            //write to memory
            printf("unlock");
            //sb.sem_op = 1;
            //if (semop(semid, &sb, 1) == -1) {
              //  perror("failed acting semaphor action");
                //todo ReleaseMemoryEndExit();
            //}
        }
    } while (1);


    return 0;
}

#pragma clang diagnostic pop

void EndProgram() {

}