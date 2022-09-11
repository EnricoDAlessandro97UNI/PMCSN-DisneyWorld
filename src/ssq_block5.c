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
#include "orchestrator_helper.h"


#define START         0.0              /* initial time                   */
#define THRESHOLD_PROBABILITY 0.4        /* failure probability */

#define M5 40




double GetArrivalBlock5(int blockNum)
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


double GetServiceBlock5()
/* --------------------------------------------
 * generate the next service time with rate 2/3
 * --------------------------------------------
 */
{
    SelectStream(5);
    return (Exponential(M5));
}




void *block5() {
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

    float prob = 0;
    int forwarded = 0;

    double lastArrival = 0.0;

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


    while ((stopFlag != 4) || (arrivalsBlockFive != NULL) || (number > 0)) {

        /* Wait for the start from the orchestrator */
        oper.sem_num = 4;
        oper.sem_op = -1;
        oper.sem_flg = 0;
        semop(sem, &oper, 1);

        if (stopFlag2 == 1){
            break;
        }

        //printf("\n-------- BLOCK 5 --------\n");

        nextEvent = get_next_event_type(5);
        //printf("\nBLOCK 5 type event %d\n", nextEvent);
        if (nextEvent == 0) {
            t.arrival = get_next_event_time(5);
            //printf("\nBLOCK 5 ARRIVAL %f\n", t.arrival);
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

            t.arrival = GetArrivalFromQueue(5)->time;
            lastArrival = t.arrival;
            DeleteFirstArrival(5);


            if (number == 1) {
                t.completion = t.current + GetServiceBlock5();
                //printf("service %6.2f completion %6.2f\n", t.completion - t.current, t.completion);
            }

            t.arrival = INFINITY;

        } else {                                        /* process a completion */
            //printf("\nBLOCK5: Processing a departure...\n");

            //printf("\nDeparture time  %6.2f\n", t.completion);

            dt = t.current;

            index++;
            number--;
            if (number > 0) {
                t.completion = t.current + GetServiceBlock5();
                //printf("service for next event: %6.2f\n", t.completion - t.current);

            }else {
                t.completion = INFINITY;
            }

            /* Return departure to the orchestrator */
            departureInfo.blockNum = 5;
            departureInfo.time = dt;

        }

        t.next = Min(t.arrival, t.completion);  /* next event time   */
        update_next_event(5, t.next, (t.next == t.arrival) ? 0 : 1); /* (t.next == t.arrival) ? 0 : 1 significa che se e è uguale a 0 allora passa 0 (arrivo) altrimenti passa 1 (partenza) */

        //printf("--------------------------\n\n");


        oper.sem_num = 0;
        oper.sem_op = 1;
        oper.sem_flg = 0;
        semop(mainSem, &oper, 1);

    }

    whoIsFree[4] = 1;
    stopFlag = 5;
    update_next_event(5, INFINITY, -1);
    printf("\nBLOCK5: Terminated, waiting for the orchestrator...\n");

    oper.sem_num = 4;
    oper.sem_op = -1;
    oper.sem_flg = 0;
    semop(sem,&oper,1);

    printf("\nBLOCK 5 STATISTICS:");

    printf("\nfor %ld jobs\n", index);
    printf("   average interarrival time = %6.2f\n", lastArrival / index);
    printf("   average wait ............ = %6.2f\n", area.node / index);
    printf("   average delay ........... = %6.2f\n", area.queue / index);
    printf("   average service time .... = %6.2f\n", area.service / index);
    printf("   average # in the node ... = %6.2f\n", area.node / t.current);
    printf("   average # in the queue .. = %6.2f\n", area.queue / t.current);
    printf("   utilization ............. = %6.2f\n", area.service / t.current);

    return (0);
}
