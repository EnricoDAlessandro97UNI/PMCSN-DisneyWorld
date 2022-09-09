#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>

#include "orchestrator_helper.h"
#include "block1_tickets/block1_helper.h"
#include "block2_dis/block2_helper.h"
#include "block3_norm/block3_helper.h"
#include "block4_contr/block4_helper.h"
#include "block5_storage/block5_helper.h"

/*
 * gcc -Wall -Wextra orchestrator.c global_helper.c block1_tickets/msq_block1.c block1_tickets/block1_helper.c block2_dis/ssq1_block2.c block2_dis/block2_helper.c block3_norm/msq_block3.c block3_norm/block3_helper.c block4_contr/msq_block4.c block4_contr/block4_helper.c rngs.o -o simu -lm -lpthread
 */

pthread_t tid[5];

//int feedback_counter;

glbl_info *glblHead;

/* ----------- GLOBAL VARIABLES ----------- */
global_info globalInfo[4];
block_queue *glblDep;
block_queue *arrivalsBlockOne;
block_queue *arrivalsBlockTwo;
block_queue *arrivalsBlockThree;
block_queue *arrivalsBlockFour;
block_queue *arrivalsBlockFive;
departure_info departureInfo;

int sem;
int mainSem;

int stopFlag;
/* ---------------------------------------- */



/* Creation of threads for block management */
void create_threads() {

    int ret;

    ret = pthread_create(&tid[0], NULL, block1, NULL);
    if(ret != 0){
        perror("pthread create error\n");
        exit(1);
    }
    
    ret = pthread_create(&tid[1], NULL, block2, NULL);
    if(ret != 0){
        perror("pthread create error\n");
        exit(1);
    }
    
    ret = pthread_create(&tid[2], NULL, block3, NULL);
    if(ret != 0){
        perror("pthread create error\n");
        exit(1);
    }   
    
    ret = pthread_create(&tid[3], NULL, block4, NULL);
    if(ret != 0){
        perror("pthread create error\n");
        exit(1);
    }

    ret = pthread_create(&tid[4], NULL, block5, NULL);
    if(ret != 0){
        perror("pthread create error\n");
        exit(1);
    }

}

int main() {
	int blockNumber;
    int i;

    int passedToThree = 0;

    struct sembuf oper;

    key_t key = IPC_PRIVATE;
    key_t mkey = IPC_PRIVATE;

    /* Initializes main semaphore */
    mainSem = semget(mkey,1,IPC_CREAT|0666);
    if(sem == -1) {
        printf("semget error\n");
        exit(EXIT_FAILURE);
    }
    semctl(sem, 0, SETVAL, 0);

    /* Initializes semaphore for threads */
    sem = semget(key,5,IPC_CREAT|0666);
    if(sem == -1) {
        printf("semget error\n");
        exit(EXIT_FAILURE);
    }

    for (i=0; i<5; i++) {
        semctl(sem, i, SETVAL, 0);
    }

    //feedback_counter = 0;

    glblDep = NULL;
    departureInfo.time = -1;

    /* Create the global event list */
    //create_list();
    //init_list();

    /* Init global_info structure */
    init_global_info_structure();
    /* Init arrivals queues */
    init_queues();

    /* Creation of threads */
    create_threads();

    int three = 0;

    /* Waiting for threads creation */
    oper.sem_num = 0;
    oper.sem_op = -5;
    oper.sem_flg = 0;
    semop(mainSem,&oper,1);

    while(1) {
       
        blockNumber = get_next_event();  /* Takes the index of the block with the most imminent event */
        switch(blockNumber) {

            case 1:
                oper.sem_num = 0;
                oper.sem_op = 1;
                oper.sem_flg = 0;
                semop(sem,&oper,1);
                break;

            case 2:
                oper.sem_num = 1;
                oper.sem_op = 1;
                oper.sem_flg = 0;
                semop(sem,&oper,1);
                break;

            case 3:
                oper.sem_num = 2;
                oper.sem_op = 1;
                oper.sem_flg = 0;
                semop(sem,&oper,1);
                break;

            case 4:
                oper.sem_num = 3;
                oper.sem_op = 1;
                oper.sem_flg = 0;
                semop(sem,&oper,1);
                break;

            case 5:
                oper.sem_num = 4;
                oper.sem_op = 1;
                oper.sem_flg = 0;
                semop(sem,&oper,1);
                break;

            case -1:
                printf("\nORCHESTRATOR: Error get_next_event\n");
                exit(EXIT_FAILURE); 


            default:
                continue;
        }

        /* Waiting for thread operation */
        oper.sem_num = 0;
        oper.sem_op = -1;
        oper.sem_flg = 0;
        semop(mainSem, &oper, 1);

        /* Se il blocco che ha appena terminato ha processato
         * una partenza, questa deve essere posta come arrivo per il blocco
         * successivo. Ogni thead deve quindi 'restituire' qualcosa al
         * orchestrator in modo tale che esso sappia se deve inserire un nuovo
         * arrivo nella lista di un blocco */
        if (departureInfo.time != -1) {  /* vuol dire che c'è stata una partenza dal blocco che ha appena terminato quindi un arrivo nel blocco successivo */

            printf("\tORCHESTRATOR: Passing departure %6.2f to block %d\n", departureInfo.time, (departureInfo.blockNum)+1);


            add_event_to_queue(departureInfo.time, (departureInfo.blockNum) + 1);

            departureInfo.time = -1;
        }

        printf("\n\n #www stopFlag %d\n\n", stopFlag);
        if (stopFlag == 5) {

            /*oper.sem_num = 4;
            oper.sem_op = 1;
            oper.sem_flg = 0;

            semop(sem, &oper, 1);
            */
            printf("\n\n| ------------------ STATISTICS ------------------ |\n");

            /* sblocco tutti i thread in ordine in modo che terminino. In questo modo però i blocchi non terminano tutti i job che hanno in coda */
            for(int i = 0; i < 5; i++) {
                oper.sem_num = i;
                oper.sem_op = 1;
                oper.sem_flg = 0;

                semop(sem, &oper, 1);
                pthread_join(tid[i], NULL);
                printf("\nBLOCK %d JOINED\n", i+1);
            }

            break; /* stop the cycle */
        }        
    }

    printf("\n| ------------------------------------------------ |\n\n");


    exit(0);
}