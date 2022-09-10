/* ------------------------------------------------------------------------- 
 * This program is a next-event simulation of a single-server FIFO service
 * node using Exponentially distributed interarrival times and Erlang 
 * distributed service times (i.e., a M/E/1 queue).  The service node is 
 * assumed to be initially idle, no arrivals are permitted after the 
 * terminal time STOP, and the node is then purged by processing any 
 * remaining jobs in the service node.
 *
 * Name            : ssq4.c  (Single Server Queue, version 4)
 * Author          : Steve Park & Dave Geyer
 * Language        : ANSI C
 * Latest Revision : 11-09-98
 * ------------------------------------------------------------------------- 
 */

#include <stdio.h>
#include <math.h>
#include <sys/sem.h>
#include "../orchestrator_helper.h"


#define START         0.0              /* initial time                   */

#define M2 20


double GetArrivalBlock2(int blockNum)
/* ---------------------------------------------
 * generate the next arrival time, with rate 1/2
 * ---------------------------------------------
 */
{
    /*static double arrival = START;

    SelectStream(2);
    arrival += Exponential(2.0);
    return (arrival);*/
    return globalInfo[blockNum - 1].time;
}


double GetServiceBlock2()
/* --------------------------------------------
 * generate the next service time with rate 2/3
 * --------------------------------------------
 */
{
    SelectStream(2);
    return (Exponential(M2));
}




void *block2() {
    struct {
        double arrival;                 /* next arrival time                   */
        double completion;              /* next completion time                */
        double current;                 /* current time                        */
        double next;                    /* next (most imminent) event time     */
        double last;                    /* last arrival time                   */
    } t;
    struct {
        double node;                    /* time integrated number in the node  */
        double queue;                   /* time integrated number in the queue */
        double service;                 /* time integrated number in service   */
    } area = {0.0, 0.0, 0.0};
    long index = 0;                  /* used to count departed jobs         */
    long number = 0;                  /* number in the node                  */


    //PlantSeeds(0);
    t.current = START;           /* set the clock                         */
    t.arrival = INFINITY;    /* schedule the first arrival            */
    t.completion = INFINITY;        /* the first event can't be a completion */


    int nextEvent;        /* Next event type */
    double dt = 0;

    struct sembuf oper;
    /* Unlock the orchestrator */
    oper.sem_num = 0;
    oper.sem_op = 1;
    oper.sem_flg = 0;
    semop(mainSem, &oper, 1);


    while ((stopFlag != 1) || (arrivalsBlockTwo != NULL) || (number > 0)) {

        /* Wait for the start from the orchestrator */
        oper.sem_num = 1;
        oper.sem_op = -1;
        oper.sem_flg = 0;
        semop(sem, &oper, 1);

        if (stopFlag2 == 1){
            break;
        }

        //printf("\n-------- BLOCK 2 --------\n");

        nextEvent = get_next_event_type(2);
        //printf("\nBLOCK 2 type event %d\n", nextEvent);
        if (nextEvent == 0) {
            t.arrival = get_next_event_time(2);
            //printf("\nBLOCK 2 ARRIVAL %f\n", t.arrival);
        }


        t.next = Min(t.arrival, t.completion);  /* next event time   */
        if (number > 0 && t.next != INFINITY) {                               /* update integrals  */
            area.node += (t.next - t.current) * number;
            area.queue += (t.next - t.current) * (number - 1);
            area.service += (t.next - t.current);
        }
        t.current = t.next;                    /* advance the clock */

        //if (t.current == t.arrival) {
        if(nextEvent == 0){                     /* process an arrival */
            number++;

            t.arrival = GetArrivalFromQueue(2)->time;
            DeleteFirstArrival(2);


            if (number == 1) {
                t.completion = t.current + GetServiceBlock2();
                //printf("service %6.2f completion %6.2f\n", t.completion - t.current, t.completion);
            }

            t.arrival = INFINITY;

        } else {                                        /* process a completion */
            //printf("\nBLOCK2: Processing a departure...\n");

            //printf("\nDeparture time  %6.2f\n", t.completion);

            dt = t.current;

            index++;
            number--;
            if (number > 0) {
                t.completion = t.current + GetServiceBlock2();
                //printf("service for next event: %6.2f\n", t.completion - t.current);

            }else {
                t.completion = INFINITY;
            }

            /* Return departure to the orchestrator */
            departureInfo.blockNum = 2;
            departureInfo.time = dt;

        }

        t.next = Min(t.arrival, t.completion);  /* next event time   */
        update_next_event(2, t.next, (t.next == t.arrival) ? 0 : 1); /* (t.next == t.arrival) ? 0 : 1 significa che se e è uguale a 0 allora passa 0 (arrivo) altrimenti passa 1 (partenza) */

        //printf("--------------------------\n\n");


        oper.sem_num = 0;
        oper.sem_op = 1;
        oper.sem_flg = 0;
        semop(mainSem, &oper, 1);

    }

    whoIsFree[1] = 1;
    /* siccome il blocco 4 deve attendere sia il 2 che il tre allora è necessario aggiungere questo valore e fargli aspettare finche non diventi 6 */
    stopFlag += 2;
    update_next_event(2, INFINITY, -1);
    //printf("\nBLOCK2: Terminated, waiting for the orchestrator...\n");

    oper.sem_num = 1;
    oper.sem_op = -1;
    oper.sem_flg = 0;
    semop(sem,&oper,1);

    printf("\nBLOCK 2 STATISTICS:");

    printf("\nfor %ld jobs\n", index);
    printf("   average interarrival time = %6.2f\n", t.last / index);
    printf("   average wait ............ = %6.2f\n", area.node / index);
    printf("   average delay ........... = %6.2f\n", area.queue / index);
    printf("   average service time .... = %6.2f\n", area.service / index);
    printf("   average # in the node ... = %6.2f\n", area.node / t.current);
    printf("   average # in the queue .. = %6.2f\n", area.queue / t.current);
    printf("   utilization ............. = %6.2f\n", area.service / t.current);

    return (0);
}
