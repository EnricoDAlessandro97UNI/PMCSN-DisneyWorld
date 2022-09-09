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


/*
* Questa è la simulazione per il blocco 4.
* Si tratta di un multiserver con coda finita, quindi ho modificato msq.c per far si che vengano scartati i job se la coda è piena.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <pthread.h>

#include "../orchestrator_helper.h"
#include "block4_helper.h"
#include "../rngs.h"

#define START    0.0                    /* initial (open the door)        */
#define SERVERS_FOUR  4                      /* number of servers              */
#define THRESHOLD_PROBABILITY 0.4        /* failure probability */

typedef struct
{             /* the next-event list    */
    double t; /*   next event time      */
    int x;    /*   event status, 0 or 1 */
} event_list_four[SERVERS_FOUR + 1];

int feedback_counter;

double GetServiceBlockFour(void)
/* ---------------------------------------------
 * generate the next service time, with rate 1/6
 * ---------------------------------------------
 */
{
    SelectStream(4);
    return (Exponential(2.0));
}

int NextEventBlockFour(event_list_four event)
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

    while (i < SERVERS_FOUR)
    {        /* now, check the others to find which  */
        i++; /* event type is most imminent          */
        if ((event[i].x == 1) && (event[i].t < event[e].t))
            e = i;
    }

    return (e);
}

int FindOneBlockFour(event_list_four event)
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
    while (i < SERVERS_FOUR)
    {        /* now, check the others to find which   */
        i++; /* has been idle longest                 */
        if ((event[i].x == 0) && (event[i].t < event[s].t))
            s = i;
    }
    return (s);
}


void *block4() {

    struct {
        double current;                  /* current time                       */
        double next;                     /* next (most imminent) event time    */
    } t;

    event_list_four event;

    long number = 0;             /* number in the node                 */
    int e;                      /* next event index                   */
    int s;                      /* server index                       */
    long index = 0;             /* used to count processed jobs       */
    double area = 0.0;           /* time integrated number in the node */
    struct {                           /* accumulated sums of                */
        double service;                  /*   service times                    */
        long served;                   /*   number served                    */
    } sum[SERVERS_FOUR + 1];

    int nextEvent;
    int forwarded = 0;
    double lastArrival;
    double dt = 0;
    float prob = 0;

    /* Initializes arrival event */
    t.current = START;
    event[0].t = 0;
    event[0].x = 1;

    /* Initializes server status */
    for (s = 1; s <= SERVERS_FOUR; s++) {
        event[s].t = START;          /* this value is arbitrary because */
        event[s].x = 0;              /* all servers are initially idle  */
        sum[s].service = 0.0;
        sum[s].served = 0;
    }

    struct sembuf oper;
    /* Unlock the orchestrator */
    oper.sem_num = 0;
    oper.sem_op = 1;
    oper.sem_flg = 0;
    semop(mainSem, &oper, 1);

    block_queue *arrival = NULL;

    while ((stopFlag != 3) || (number > 0) || (arrivalsBlockFour != NULL)) {

        /* Wait for the start from the orchestrator */
        oper.sem_num = 3;
        oper.sem_op = -1;
        oper.sem_flg = 0;
        semop(sem,&oper,1);

        printf("\n-------- BLOCK 4 --------\n");

        /* Find next event index */
        nextEvent = get_next_event_type(4);
        if (nextEvent == 0) {
            event[0].t = get_next_event_time(4);
        }

        e = NextEventBlockFour(event);
        t.next = event[e].t;
        area += (t.next - t.current) * number; /* update integral  */
        t.current = t.next;  /* advance the clock*/

        if (e == 0) {   /* Process an arrival */

            printf("\nBLOCK4: Processing arrival...\n");

            arrival = GetArrivalFromQueue(4);
            lastArrival = arrival->time;
            printf("\nt.current %6.2f = arrival %6.2f\n", t.current, arrival->time);
            DeleteFirstArrival(4);

            /* Set next arrival event */
            if (arrivalsBlockFour == NULL) { /* ArrivalBlockFour queue empty */
                printf("\nBlock four NULL\n");
                event[0].t = INFINITY;
                event[0].x = 1;
            }
            else {
                event[0].t = arrival -> time;
                event[0].x = 1;
            }
            
            if (number < SERVERS_FOUR) {  /* controlla se ci sono server liberi */
                number++;       /* devo incrementare solo quelli che vengono accettati */
                double service = GetServiceBlockFour();
                printf("\tService: %6.2f\n", service);
                s = FindOneBlockFour(event);
                sum[s].service += service;
                sum[s].served++;
                event[s].t = t.current + service;
                event[s].x = 1;
                printf("\tBLOCK4: Accepted --> Terminates: %lf\n", event[s].t);
            } 
            else {
                printf("\tBLOCK4: Forwarded\n");
                forwarded++;
                /* il job non viene preso in carico e deve essere mandato al successivo blocco */
                /* prepara il ritorno da dare all'orchestrator */
                
                departureInfo.blockNum = 4;
                departureInfo.time = t.current;
                printf("\tDeparture: %6.2f\n", t.current);
            }
        }
        else {  /* Process a departure */

            printf("\nBLOCK4: Processing a departure...\n");
    
            index++;
            number--; /* Job completed */
            s = e;

            /* Feedback */
            prob = get_failure_probability();
            printf("\nPROB: %f\n", prob);
            if (prob < THRESHOLD_PROBABILITY) { /* Feedback */
                printf("\nPROB: feedback\n");
                feedback_counter++;  /* set the feedback flag for the orchestrator */
            } 
            else { /* No feedback */
                printf("\tDeparture: %6.2f\n", event[s].t);
                departureInfo.blockNum = 4;
                departureInfo.time = event[s].t;
            }

            event[s].x = 0; /* Server now idle */
            event[s].t = INFINITY;
        }

        e = NextEventBlockFour(event);  /* next event index */
        update_next_event(4, event[e].t, (e == 0) ? 0 : 1);     /* (e == 0) ? 0 : 1 significa che se e è uguale a 0 allora passa 0 altrimenti passa 1 */

        printf("--------------------------\n\n");

        oper.sem_num = 0;
        oper.sem_op = 1;
        oper.sem_flg = 0;
        semop(mainSem,&oper,1);
    }

    stopFlag = 4;
    update_next_event(4, INFINITY, -1);
    printf("\nBLOCK4: Terminated, waiting for the orchestrator...\n");

    oper.sem_num = 3;
    oper.sem_op = -1;
    oper.sem_flg = 0;
    semop(sem,&oper,1);

    printf("BLOCK 4 STATISTICS:");

    printf("\n\nfor %ld jobs, forwarded %d\n", index, forwarded);
    printf("  avg interarrivals .. = %6.2f\n", lastArrival / index);
    printf("  avg wait ........... = %6.2f\n", area / index);
    printf("  avg # in node ...... = %6.2f\n", area / t.current);

    for (s = 1; s <= SERVERS_FOUR; s++)            /* adjust area to calculate */
        area -= sum[s].service;              /* averages for the queue   */

    printf("  avg delay .......... = %6.2f\n", area / index);
    printf("  avg # in queue ..... = %6.2f\n", area / t.current);
    printf("\nthe server statistics are:\n\n");
    printf("    server     utilization     avg service        share\n");
    for (s = 1; s <= SERVERS_FOUR; s++)
        printf("%8d %14.3f %15.2f %15.3f\n", s, sum[s].service / t.current,
               sum[s].service / sum[s].served,
               (double) sum[s].served / index);
    printf("\n");

    printf("# of feedback %d\n", feedback_counter);

    pthread_exit((void *)0);

}
