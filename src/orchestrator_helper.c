/**
 * Authors: Alessandro De Angelis & Enrico D'Alessandro
 * 
 */
#include <math.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <pthread.h>
#include "rngs.h"
#include "orchestrator_helper.h"


void update_availability(int block) {
    /* cerco il blocco */
    glbl_info *t = glblHead;
    while(t->blockNum != block){
        t = t->next;
    }
    printf("\nBlock %d next_e_t %6.2f\n", t->blockNum, t->ne->time);
       /* bisogna evitare di farlo se in coda c'è un solo arrivo */
    t->available_event_flag = 1;
}


int get_available_e_flag(int block){
    glbl_info *t = glblHead;
    while(t != NULL) {
        if(t->blockNum == block){
            return t->available_event_flag;
        }
        t = t->next;
    }

    return -1;
}


int set_available_e_flag(int block){
    glbl_info *t = glblHead;
    while(t != NULL){
        if(t->blockNum == block){
            t->available_event_flag = 0;
            return t->available_event_flag;
        }
        t = t->next;
    }
    return -1;

}

/*
void update_next_event(int block, double time, int k){
    glbl_info *t = glblHead;
    while (t != NULL) {
        if (t -> blockNum == block) {
            t -> ne -> time = time;     // istante prossimo evento 
            t -> ne -> eventType = k;      // tipo del prossimo evento (0 arrivo, 1 partenza) 
            break;
        }
        t = t -> next;
    }
}
*/

void update_next_event(int blockNum, double time, int eventType) {
    globalInfo[blockNum-1].time = time;             /* Instant of the next event */
    globalInfo[blockNum-1].eventType = eventType;   /* Next event type: 0 arrival, 1 departure */
}

void init_queues() {
    arrivalsBlockOne = NULL;
    arrivalsBlockTwo = NULL;
    arrivalsBlockThree = NULL;
    arrivalsBlockFour = NULL;
    arrivalsBlockFive = NULL;
}

/* quando c'è un nuovo arrivo in un blocco bisogna aggiornare il next event */
/*
void update_next_event_after_arrival(int block, double time) {
    glbl_info *gh = glblHead;
    int index = 1;
    while (index != block){
        gh = gh -> next;
        index++;
    }
    if(gh -> ne -> time > time) { // questo nuovo evento avviene prima di quello schedulato in precedenza dal blocco, quindi cambio il next event
        gh -> ne -> time = time;
        gh -> ne -> eventType = 0; // the event is an arrival
    }
}
*/
void update_next_event_after_arrival(int blockNum, double time) {
    if (globalInfo[blockNum-1].time > time) { // change the next event for the block blockNum
        globalInfo[blockNum-1].time = time;
        globalInfo[blockNum-1].eventType = 0; // the event is an arrival
    }
}


int add_event_to_queue(double time, int block) {

    block_queue *tmp;

    block_queue *new = (block_queue *) malloc(sizeof(block_queue));
    if (new == NULL) {
        perror("Malloc error:");
        exit(-1);
    }
    new -> block = block;
    new -> time = time;
    new -> next = NULL;

    switch (block) {

        case 1:
            if (arrivalsBlockOne != NULL) {
                tmp = arrivalsBlockOne;
            } else {
                arrivalsBlockOne = new;
                tmp = arrivalsBlockOne;
                update_next_event_after_arrival(1, time);
                return 0;
            }
            break;

        case 2:
            if (arrivalsBlockTwo != NULL) {
                tmp = arrivalsBlockTwo;
            } else {
                arrivalsBlockTwo = new;
                tmp = arrivalsBlockTwo;
                update_next_event_after_arrival(2, time);
                return 0;
            }
            break;

        case 3:
            if (arrivalsBlockThree != NULL) {
                tmp = arrivalsBlockThree;
            } else {
                arrivalsBlockThree = new;
                tmp = arrivalsBlockThree;
                update_next_event_after_arrival(3, time);
                return 0;
            }
            return 0;
            break;

        case 4:
            if (arrivalsBlockFour != NULL) {
                tmp = arrivalsBlockFour;
            } else {
                arrivalsBlockFour = new;
                tmp = arrivalsBlockFour;
                update_next_event_after_arrival(4, time);
                return 0;
            }
            return 0;
            break;

        case 5:
            if (arrivalsBlockFive != NULL) {
                tmp = arrivalsBlockFive;
            } else {
                arrivalsBlockFive = new;
                tmp = arrivalsBlockFive;
                update_next_event_after_arrival(5, time);
                return 0;
            }
            return 0;

        default:
            /* significa che è una partenza dal blocco 5 quindi esce dal sistema */
            return -1;
    }

    /* bisogna aggiornare il prossimo evento in caso questo arrivo avvenga prima */
    while (tmp -> next != NULL) {
        tmp = tmp -> next;
    }
    tmp -> next = new;

    update_next_event_after_arrival(block, time);

    return 0;
}


/* Return the index of the block with the most imminent event */
/*
int get_next_event() {
    glbl_info *t = glblHead;
    int block = -1;
    double min = glblHead -> ne -> time;
    block = 1;
    while (t != NULL){
        printf("Block %d next event time: %6.2f\n", t -> blockNum, t -> ne -> time);
        if(t -> ne -> time < min){
            block = t -> blockNum;
            min = t -> ne -> time;
        }
        t = t -> next;
    }

    printf("\nMin time: %6.2f\n", min);
    return block;   // -1 =: ERROR 
}
*/

/* Return the index of the block with the most imminent event */
int get_next_event() {
    int blockNumber = 1;
    double min = globalInfo[0].time;
    for (int i=0; i<5; i++) {
        //printf("Block %d next event time: %6.2f\n", i+1, globalInfo[i].time);
        if (globalInfo[i].time < min) {
            blockNumber = i+1;
            min = globalInfo[i].time;
        }

    }

    //printf("\nMin event time block %d: %6.2f\n", blockNumber, min);
    if(min == INFINITY){
        return -2;
    }else {
        return blockNumber;
    }
}

void create_list() {
    /* inizializza la testa della coda */
    glblHead = (glbl_info*)malloc(sizeof(glbl_info));
    if(glblHead == NULL){
        perror("Malloc error:");
        exit(-1);
    }

    glblHead->next = NULL;
    glblHead->ne = (next_event *)malloc(sizeof(next_event));
    if(glblHead->ne == NULL){
        perror("Malloc error:");
        exit(-1);
    }

    glbl_info *tmp = glblHead;

    /* crea gli altri 4 blocchi della coda */
    for(int i = 0; i < 4; i++){
        tmp->next = (glbl_info*)malloc(sizeof(glbl_info));
        if(tmp->next == NULL){
            perror("Malloc error:");
            exit(-1);
        }
        tmp->next->ne = (next_event *)malloc(sizeof(next_event));
        if(tmp->next->ne == NULL){
            perror("Malloc error:");
            exit(-1);
        }

        tmp = tmp->next;
    }
    
    tmp->next = NULL;
}


void init_list() {
    int c = 1;
    glbl_info *tmp = glblHead;
    tmp->ne->time = 0;    /* il primo blocco deve avere tempo minore di tutti */
    tmp->ne->eventType = 0;    /* all'inizio tutti i blocchi attendono un arrivo */
    tmp->blockNum = c;
    tmp->available_event_flag = 0;

    tmp = tmp->next;
    c++;
    while(tmp != NULL){
        tmp->ne->time = INFINITY;
        tmp->ne->eventType = 0;    /* all'inizio tutti i blocchi attendono un arrivo */
        tmp->blockNum = c;
        tmp->available_event_flag = 0;

        tmp = tmp->next;
        c++;
    }
}

/* Initializes the initial state of the global_info structure  */
void init_global_info_structure() {
    /* Initializes first block */
    whoIsFree[0] = 0;
    globalInfo[0].time = 0;         /* The first block must have the most immininent event */
    globalInfo[0].eventType = 0;    /* At the beginning all the blocks await an arrival */

    /* Initializes other blocks */
    for (int i=1; i<5; i++) {
        globalInfo[i].time = INFINITY;  /* The other blocks have an infinite time  */
        globalInfo[i].eventType = 0;    /* At the beginning all the blocks await an arrival */
        whoIsFree[i] = 0;
    }
}

double Exponential(double m)
/* ---------------------------------------------------
 * generate an Exponential random variate, use m > 0.0
 * ---------------------------------------------------
 */
{
    return (-m * log(1.0 - Random()));
}


double Uniform(double a, double b)
/* --------------------------------------------
 * generate a Uniform random variate, use a < b
 * --------------------------------------------
 */
{
    return (a + (b - a) * Random());
}

block_queue* GetArrivalFromQueue(int blockNum) {
    
    switch (blockNum) {

        case 1:
            return arrivalsBlockOne;
            break;

        case 2:
            return arrivalsBlockTwo;
            break;

        case 3:
            return arrivalsBlockThree;
            break;

        case 4:
            return arrivalsBlockFour;
            break;

        case 5:
            return arrivalsBlockFive;
            break;

        default:
            printf("ERROR: wrong code number\n");
            exit(EXIT_FAILURE);
    }

    return NULL;
}

int DeleteFirstArrival(int blockNum) {

    block_queue *tmp;
    switch (blockNum) {

        case 1:
            tmp = arrivalsBlockOne;
            arrivalsBlockOne = arrivalsBlockOne -> next;
            free(tmp);
            tmp = NULL;
            break;

        case 2:
            tmp = arrivalsBlockTwo;
            arrivalsBlockTwo = arrivalsBlockTwo -> next;
            free(tmp);
            tmp = NULL;
            break;

        case 3:
            tmp = arrivalsBlockThree;
            arrivalsBlockThree = arrivalsBlockThree -> next;
            free(tmp);
            tmp = NULL;
            break;

        case 4:
            tmp = arrivalsBlockFour;
            arrivalsBlockFour = arrivalsBlockFour -> next;
            free(tmp);
            tmp = NULL;
            break;

        case 5:
            tmp = arrivalsBlockFive;
            arrivalsBlockFive = arrivalsBlockFive -> next;
            free(tmp);
            tmp = NULL;
            break;

        default:
            printf("ERROR: wrong code number\n");
            exit(EXIT_FAILURE);
    }

    return 0;
}

int get_next_event_type(int blockNum) {
    return globalInfo[blockNum-1].eventType;
}

double get_next_event_time(int blockNum) {
    return globalInfo[blockNum-1].time;
}


float get_probability() {
    SelectStream(6);
    return (float)(Uniform(0.0, 1.0));
}

double Min(double a, double c)
/* ------------------------------
 * return the smaller of a, b
 * ------------------------------
 */
{
    if (a < c)
        return (a);
    else
        return (c);
}

float get_forward_probability() {
    SelectStream(7);
    return (float)(Uniform(0.0, 1.0));
}

double Erlang(long n, double b)
/* ==================================================
 * Returns an Erlang distributed positive real number.
 * NOTE: use n > 0 and b > 0.0
 * ==================================================
 */
{
    long   i;
    double x = 0.0;

    for (i = 0; i < n; i++)
        x += Exponential(b);
    return (x);
}


int unlock_waiting_threads(){
    struct sembuf oper;
    int c = 0;
/* sblocco tutti i thread in ordine in modo che terminino. In questo modo però i blocchi non terminano tutti i job che hanno in coda */
    for(int i = 0; i < 5; i++) {
        if(whoIsFree[i] == 0) {
            oper.sem_num = i;
            oper.sem_op = 1;
            oper.sem_flg = 0;

            semop(sem, &oper, 1);
            //printf("\nBLOCK %d going to the end\n", i + 1);
            c++;
        }
    }
    return 0;
}