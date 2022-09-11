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

#include "block5_helper.h"
#include "../finite_helper.h"
#include "../rngs.h"

#define START 0.0          /* initial (open the door)        */
#define SERVERS_FIVE 7    /* number of servers              */

#define M5 60

typedef struct
{             /* the next-event list    */
    double t; /*   next event time      */
    int x;    /*   event status, 0 or 1 */
} event_list_five[SERVERS_FIVE + 1];

double GetServiceBlockFive(void)
/* ---------------------------------------------
 * generate the next service time, with rate 1/6
 * ---------------------------------------------
 */
{
    SelectStream(5);
    return (Exponential(M5));
}

int NextEventBlockFive(event_list_five event)
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

    while (i < SERVERS_FIVE)
    {        /* now, check the others to find which  */
        i++; /* event type is most imminent          */
        if ((event[i].x == 1) && (event[i].t < event[e].t))
            e = i;
    }

    return (e);
}

int FindOneBlockFive(event_list_five event)
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
    while (i < SERVERS_FIVE)
    {        /* now, check the others to find which   */
        i++; /* has been idle longest                 */
        if ((event[i].x == 0) && (event[i].t < event[s].t))
            s = i;
    }
    return (s);
}

void *block5() {

    FILE *fp;

    struct {
        double current;      /* current time                       */
        double next;         /* next (most imminent) event time    */
    } t;

    event_list_five event;

    long number = 0;      /* number in the node                 */
    int e;                /* next event index                   */
    int s;                /* server index                       */
    long index = 0;       /* used to count processed jobs       */
    double area = 0.0;    /* time integrated number in the node */

    struct {                   /* accumulated sums of */
        double service;        /*   service times     */
        long served;           /*   number served     */
    } sum[SERVERS_FIVE + 1];

    int nextEvent;        /* Next event type */
    double lastArrival;
    double dt = 0;
    double service;

    double totalService = 0.0;
    double avgService = 0.0;
    double totalUtilization = 0.0;

    /* Initialize arrival event */
    t.current = START;
    event[0].t = 0;
    event[0].x = 1;

    /* Initialize server status */
    for (s = 1; s <= SERVERS_FIVE; s++) {
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

    while ((stopFlag != 4) || (number > 0) || (arrivalsBlockFive != NULL)) {

        /* Wait for the start from the orchestrator */
        oper.sem_num = 4;
        oper.sem_op = -1;
        oper.sem_flg = 0;
        semop(sem, &oper, 1);

        if (stopFlag2 == 1){
            break;
        }

        //printf("\n-------- BLOCK 5 --------\n");

        /* Find next event index */
        nextEvent = get_next_event_type(5);
        //printf("\nBLOCK 5 type event %d\n", nextEvent);
        if (nextEvent == 0) {
            event[0].t = get_next_event_time(5);
            //printf("\nBLOCK 5 ARRIVAL %f\n", event[0].t);
        }

        e = NextEventBlockFive(event);
        t.next = event[e].t;
        area += (t.next - t.current) * number; /* update integral  */
        t.current = t.next;  /* advance the clock*/



        if (e == 0) {   /* Process an arrival */
            //printf("\nBLOCK5: Processing arrival %6.2f\n", t.current);
            
            number++;
            
            arrival = GetArrivalFromQueue(5);
            lastArrival = arrival->time;
            DeleteFirstArrival(5);

            /* Set next arrival event */
            if (arrivalsBlockFive == NULL) { /* ArrivalBlockFive queue empty */
                event[0].t = INFINITY;
                event[0].x = 1;
            }
            else {
                event[0].t = arrival -> time;
                event[0].x = 1;
            }

            if (number <= SERVERS_FIVE)
            { /* se nel sistema ci sono al più tanti job quanti i server allora calcola un tempo di servizio */
                service = GetServiceBlockFive();
                //printf("\tService: %6.2f\n", service);
                s = FindOneBlockFive(event);
                //printf("\tServer selected: %d\n", s);
                sum[s].service += service;
                sum[s].served++;
                event[s].t = t.current + service;
                //printf("\nNext service from server %d: %6.2f\n", s, event[s].t);
                event[s].x = 1;
            }
        }
        else {  /* Process a departure from server s */

            //printf("\nBLOCK5: Processing a departure...\n");

            index++;                         
            number--; /* Job completed */
            s = e;

            //printf("\nService from server %d\n", s);

            //printf("\tDeparture: %6.2f\n", event[s].t);
            dt = event[s].t;
            if (number >= SERVERS_FIVE) {
                service = GetServiceBlockFive();
                sum[s].service += service;
                sum[s].served++;
                event[s].t = t.current + service;
            } else {
                event[s].t = INFINITY;
                event[s].x = 0;
            }

            /* Return departure to the orchestrator */
            departureInfo.blockNum = 5;
            departureInfo.time = dt;
        }

        e = NextEventBlockFive(event);
        update_next_event(5, event[e].t, (e == 0) ? 0 : 1); /* (e == 0) ? 0 : 1 significa che se e è uguale a 0 allora passa 0 (arrivo) altrimenti passa 1 (partenza) */

        //printf("--------------------------\n\n");


        oper.sem_num = 0;
        oper.sem_op = 1;
        oper.sem_flg = 0;
        semop(mainSem, &oper, 1);
    }

    whoIsFree[4] = 1;
    //stopFlag = 5;
    update_next_event(5, INFINITY, -1);
    //printf("\nBLOCK5: Terminated, waiting for the orchestrator...\n");

    oper.sem_num = 4;
    oper.sem_op = -1;
    oper.sem_flg = 0;
    semop(sem,&oper,1);

    printf("\nBLOCK 5 STATISTICS:");
    printf("\nlast arrival: %6.2f\n", lastArrival);
    printf("\n\nfor %ld jobs\n", index);
    printf("  avg interarrivals .. = %6.2f\n", lastArrival / index);
    printf("  avg wait ........... = %6.2f\n", area / index);
    printf("  avg # in node ...... = %6.2f\n", area / t.current);

    /* Write statistics on file */
    fp = fopen(FILENAME_WAIT_BLOCK5, "a");
    fprintf(fp,"%6.6f\n", area / index);
    fclose(fp);

    for (s = 1; s <= SERVERS_FIVE; s++)     /* adjust area to calculate */
        area -= sum[s].service;              /* averages for the queue   */

    printf("  avg delay .......... = %6.2f\n", area / index);
    printf("  avg # in queue ..... = %6.2f\n", area / t.current);
    printf("\nthe server statistics are:\n\n");
    printf("    server     utilization     avg service        share\n");
    for (s = 1; s <= SERVERS_FIVE; s++) {
        printf("%8d %14.3f %15.2f %15.3f\n", s, sum[s].service / t.current,
               sum[s].service / sum[s].served,
               (double) sum[s].served / index);
        totalService += sum[s].service / sum[s].served;
        totalUtilization += sum[s].service / t.current;
    }

    avgService = totalService / SERVERS_FIVE;

    printf("\n   avg service ........ = %6.6f\n", avgService / SERVERS_FIVE);
    printf("   avg utilization .... = %6.6f\n", totalUtilization / SERVERS_FIVE);

    /* Write statistics on file */
    fp = fopen(FILENAME_DELAY_BLOCK5, "a");
    fprintf(fp,"%6.6f\n", area / index);
    fclose(fp);

    printf("\n");

    pthread_exit((void *)0);
}
