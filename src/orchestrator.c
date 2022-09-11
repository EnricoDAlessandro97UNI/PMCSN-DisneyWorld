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

/* ----------- GLOBAL VARIABLES ----------- */
pthread_t tid[5];
global_info globalInfo[5];
block_queue *arrivalsBlockOne;
block_queue *arrivalsBlockTwo;
block_queue *arrivalsBlockThree;
block_queue *arrivalsBlockFour;
block_queue *arrivalsBlockFive;
departure_info departureInfo;

int sem;
int mainSem;

int whoIsFree[5];

int stopFlag;
int stopFlag2;

int block4Lost;
int block4ToExit;
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

    printf("\nAll threads created\n");

}


int main() {
	int blockNumber;
    int i;

    int c2 = 0;
    int c3 = 0;

    struct sembuf oper;

    key_t key = IPC_PRIVATE;
    key_t mkey = IPC_PRIVATE;

    /* Initializes main semaphore */
    mainSem = semget(mkey,1,IPC_CREAT|0666);
    if(mainSem == -1) {
        printf("semget error\n");
        exit(EXIT_FAILURE);
    }

    /* qui c'erano i semctl */
    /* -------------------------------------------------------------------------------------------------------------------- */

    /* Initializes semaphore for threads */
    sem = semget(key,5,IPC_CREAT|0666);
    if(sem == -1) {
        printf("semget error\n");
        exit(EXIT_FAILURE);
    }

    /* qui c'erano i semctl */
    /* -------------------------------------------------------------------------------------------------------------------- */

    PlantSeeds(SEED);

    /* Create files for statistics */
    create_statistics_files();

    int rep = 1;
    while(rep <= 256) {

        block4Lost = 0;
        block4ToExit = 0;

        c2 = 0;
        c3 = 0;

        departureInfo.time = -1;

        /* Init semaphores */
        semctl(mainSem, 0, SETVAL, 0);

        for (i=0; i<5; i++) {
            semctl(sem, i, SETVAL, 0);
        }

        printf("\n\nSIMULATION REP: %d\n", rep);

        stopFlag = 0;
        stopFlag2 = 0;

        /* Init global_info structure */
        init_global_info_structure();
        /* Init arrivals queues */
        init_queues();

        /* Creation of threads */
        create_threads();

        float prob = 0;

        /* Waiting for threads creation */
        oper.sem_num = 0;
        oper.sem_op = -5;
        oper.sem_flg = 0;
        semop(mainSem,&oper,1);

        while (1) {
            
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

                case -2:
                    //printf("All blocks have finished, going to close\n");
                    stopFlag2 = 1;
                    int ret = unlock_waiting_threads();
                    /* attendi che quei thread terminino */
                    oper.sem_num = 0;
                    oper.sem_op = (short)-ret;
                    oper.sem_flg = 0;
                    semop(mainSem, &oper, 1);
                    break;

                default:
                    continue;
            }

            if (stopFlag2 == 1) {
                /* tutti i thread sono in attesa di stampare le statistiche, salta a quel punto */
                goto statistics;
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

                if(departureInfo.blockNum == 1){
                    prob = get_probability();
                    if(prob <= 0.1){
                        /* vuol dire che la partenza nel blocco 1 sarà un arrivo nel blocco per disabili */
                        //printf("\tORCHESTRATOR: Passing departure %6.2f to block %d\n", departureInfo.time, 2);

                        add_event_to_queue(departureInfo.time, 2);

                        departureInfo.time = -1;
                        c2++;

                    }else{
                        /* vuol dire che la partenza nel blocco 1 sarà un arrivo nel blocco 3, non per disabili*/
                        //printf("\tORCHESTRATOR: Passing departure %6.2f to block %d\n", departureInfo.time, 3);

                        add_event_to_queue(departureInfo.time, 3);

                        departureInfo.time = -1;

                        c3++;
                    }

                }else if(departureInfo.blockNum == 2 || departureInfo.blockNum == 3){

                    /* vuol dire che la partenza nel blocco 1 sarà un arrivo nel blocco per disabili */
                    //printf("\tORCHESTRATOR: Passing departure %6.2f to block %d\n", departureInfo.time, 4);

                    add_event_to_queue(departureInfo.time, 4);

                    departureInfo.time = -1;

                }else if(departureInfo.blockNum == 4){

                    prob = get_probability();
                    if(prob <= 0.3){
                        /* vuol dire che la partenza nel blocco 4 sarà un arrivo nel blocco 5 del deposito */
                        //printf("\tORCHESTRATOR: Passing departure %6.2f to block %d\n", departureInfo.time, 5);

                        add_event_to_queue(departureInfo.time, 5);

                        departureInfo.time = -1;

                    } else if(prob > 0.3 && prob <= 0.9){
                        block4ToExit++;
                        //printf("Orchestrator forwarded departure from block %d\n", departureInfo.blockNum);
                        departureInfo.time = -1;

                    }else{
                        /* è una perdita */
                        block4Lost++;
                        //printf("Orchestrator lost from block %d, time %6.2f\n", departureInfo.blockNum, departureInfo.time);
                        departureInfo.time = -1;
                    }

                }else{
                    /* vuol dire che è una partenza dal blocco 5 */
                    //printf("\tORCHESTRATOR: Passing departure %6.2f to block %d\n", departureInfo.time, departureInfo.blockNum + 1);

                    add_event_to_queue(departureInfo.time, departureInfo.blockNum + 1);

                    departureInfo.time = -1;
                }

            }

    statistics:
            //printf("\n\n #www stopFlag2 %d\n\n", stopFlag2);
            if (stopFlag2 == 1) {

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
        rep++;
    }

    exit(0);
}