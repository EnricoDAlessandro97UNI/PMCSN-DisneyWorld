/* ------------------------------------------------------------------------- 
 * This program is a next-event simulation of a multi-server, single-queue 
 * service node.  The service node is assumed to be initially idle, no 
 * arrivals are permitted after the terminal time STOP and the node is then 
 * purged by processing any remaining jobs. 
 * 
 * Name              : msq.c (Multi-Server Queue)
 * Author            : Steve Park & Dave Geyer 
 * Language          : ANSI C 
 * Latest Revision   : 10-19-98 
 * ------------------------------------------------------------------------- 
 */



#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <pthread.h>

#include "block3_helper.h"
#include "../orchestrator_helper.h"
#include "../rngs.h"

#define START 0.0          /* initial (open the door)        */
#define SERVERS_THREE 6    /* number of servers              */

#define M3 10


typedef struct
{             /* the next-event list    */
    double t; /*   next event time      */
    int x;    /*   event status, 0 or 1 */
} event_list_three[SERVERS_THREE + 1];

double GetServiceBlockThree(void)
/* ---------------------------------------------
 * generate the next service time, with rate 1/6
 * ---------------------------------------------
 */
{
    SelectStream(3);
    return (Exponential(M3));
}

int NextEventBlockThree(event_list_three event)
/* ---------------------------------------
 * return the index of the next event type
 * ---------------------------------------
 */
{
    int e;
    int i = 0;

    while (event[i].x == 0) /* find the index of the first 'active' */
        i++;                /* element in the event list            */
    e = i;

    while (i < SERVERS_THREE)
    {        /* now, check the others to find which  */
        i++; /* event type is most imminent          */
        if ((event[i].x == 1) && (event[i].t < event[e].t))
            e = i;
    }

    return (e);
}

int FindOneBlockThree(event_list_three event)
/* -----------------------------------------------------
 * return the index of the available server idle longest
 * -----------------------------------------------------
 */
{
    int s;
    int i = 1;

    while (event[i].x == 1) /* find the index of the first available */
        i++;                /* (idle) server                         */
    s = i;
    while (i < SERVERS_THREE)
    {        /* now, check the others to find which   */
        i++; /* has been idle longest                 */
        if ((event[i].x == 0) && (event[i].t < event[s].t))
            s = i;
    }
    return (s);
}

void *block3() {

    int received = 0;

    struct {
        double current;      /* current time                       */
        double next;         /* next (most imminent) event time    */
    } t;

    event_list_three event;

    long number = 0;      /* number in the node                 */
    int e;                /* next event index                   */
    int s;                /* server index                       */
    long index = 0;       /* used to count processed jobs       */
    double area = 0.0;    /* time integrated number in the node */

    struct {                   /* accumulated sums of */
        double service;        /*   service times     */
        long served;           /*   number served     */
    } sum[SERVERS_THREE + 1];

    int nextEvent;        /* Next event type */
    double lastArrival;
    double dt = 0;

    /* Initialize arrival event */
    t.current = START;
    event[0].t = 0;
    event[0].x = 1;

    /* Initialize server status */
    for (s = 1; s <= SERVERS_THREE; s++) {
        event[s].t = START;          /* this value is arbitrary because */
        event[s].x = 0;              /* all servers are initially idle  */
        sum[s].service = 0.0;
        sum[s].served = 0;
    }

    block_queue *arrival = NULL;

    struct sembuf oper;
    /* Unlock the orchestrator */
    oper.sem_num = 0;
    oper.sem_op = 1;
    oper.sem_flg = 0;
    semop(mainSem, &oper, 1);



    while ((stopFlag != 1) || (number > 0) || (arrivalsBlockThree != NULL)) {

        /* Wait for the start from the orchestrator */
        oper.sem_num = 2;
        oper.sem_op = -1;
        oper.sem_flg = 0;
        semop(sem, &oper, 1);

        if (stopFlag2 == 1){
            break;
        }

        printf("\n-------- BLOCK 3 --------\n");

        /* Find next event index */
        nextEvent = get_next_event_type(3);
        //printf("\nBLOCK 3 type event %d\n", nextEvent);
        if (nextEvent == 0) {
            event[0].t = get_next_event_time(3);
            //printf("\nBLOCK 3 ARRIVAL %f\n", event[0].t);
        }

        e = NextEventBlockThree(event);
        t.next = event[e].t;
        area += (t.next - t.current) * number; /* update integral  */
        t.current = t.next;  /* advance the clock*/



        if (e == 0) {   /* Process an arrival */
            //printf("\nBLOCK3: Processing arrival %6.2f\n", t.current);
            
            number++;
            
            arrival = GetArrivalFromQueue(3);
            lastArrival = arrival->time;
            DeleteFirstArrival(3);

            /* Set next arrival event */
            if (arrivalsBlockThree == NULL) { /* ArrivalBlockThree queue empty */
                event[0].t = INFINITY;
                event[0].x = 1;
            }
            else {
                event[0].t = arrival -> time;
                event[0].x = 1;
            }

            if (number <= SERVERS_THREE) 
            { /* se nel sistema ci sono al più tanti job quanti i server allora calcola un tempo di servizio */
                double service = GetServiceBlockThree();
                //printf("\tService: %6.2f\n", service);
                s = FindOneBlockThree(event);
                //printf("\tServer selected: %d\n", s);
                sum[s].service += service;
                sum[s].served++;
                event[s].t = t.current + service;
                //printf("\nNext service from server %d: %6.2f\n", s, event[s].t);
                event[s].x = 1;
            }
        }
        else {  /* Process a departure from server s */

            //printf("\nBLOCK3: Processing a departure...\n");

            index++;                         
            number--; /* Job completed */
            s = e;

            //printf("\nService from server %d\n", s);

            //printf("\tDeparture: %6.2f\n", event[s].t);
            dt = event[s].t;
            if (number >= SERVERS_THREE) {
                double service = GetServiceBlockThree();
                sum[s].service += service;
                sum[s].served++;
                event[s].t = t.current + service;
            } else {
                event[s].t = INFINITY;
                event[s].x = 0;
            }

            /* Return departure to the orchestrator */
            departureInfo.blockNum = 3;
            departureInfo.time = dt;
        }

        e = NextEventBlockThree(event);
        update_next_event(3, event[e].t, (e == 0) ? 0 : 1); /* (e == 0) ? 0 : 1 significa che se e è uguale a 0 allora passa 0 (arrivo) altrimenti passa 1 (partenza) */

        //printf("--------------------------\n\n");


        oper.sem_num = 0;
        oper.sem_op = 1;
        oper.sem_flg = 0;
        semop(mainSem, &oper, 1);
    }

    whoIsFree[2] = 1;
    /* siccome il blocco 4 deve attendere sia il 2 che il tre allora è necessario aggiungere questo valore e fargli aspettare finche non diventi 6 */
    stopFlag += 3;
    update_next_event(3, INFINITY, -1);
    //printf("\nBLOCK3: Terminated, waiting for the orchestrator...\n");

    oper.sem_num = 2;
    oper.sem_op = -1;
    oper.sem_flg = 0;
    semop(sem,&oper,1);

    printf("\nBLOCK 3 STATISTICS:");

    printf("\n\nfor %ld jobs\n", index);
    printf("  avg interarrivals .. = %6.2f\n", lastArrival / index);
    printf("  avg wait ........... = %6.2f\n", area / index);
    printf("  avg # in node ...... = %6.2f\n", area / t.current);

    for (s = 1; s <= SERVERS_THREE; s++)     /* adjust area to calculate */
        area -= sum[s].service;              /* averages for the queue   */

    printf("  avg delay .......... = %6.2f\n", area / index);
    printf("  avg # in queue ..... = %6.2f\n", area / t.current);
    printf("\nthe server statistics are:\n\n");
    printf("    server     utilization     avg service        share\n");
    for (s = 1; s <= SERVERS_THREE; s++)
        printf("%8d %14.3f %15.2f %15.3f\n", s, sum[s].service / t.current,
               sum[s].service / sum[s].served,
               (double) sum[s].served / index);

    printf("\n");
    printf("block 3 received %d jobs\n", received);


    pthread_exit((void *)0);
}
