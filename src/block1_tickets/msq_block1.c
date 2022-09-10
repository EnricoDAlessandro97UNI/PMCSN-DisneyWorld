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
#include <sys/sem.h>
#include <sys/mman.h>
#include <math.h>
#include <pthread.h>

#include "../orchestrator_helper.h"
#include "../rngs.h"
#include "block1_helper.h"

#define START 0.0        /* initial (open the door)        */
#define SERVERS_ONE 4    /* number of servers              */
#define STOP 2000.0     /* terminal (close the door) time */
#define THRESHOLD_PROBABILITY 0.4        /* failure probability */


typedef struct
{             /* the next-event list    */
    double t; /*   next event time      */
    int x;    /*   event status, 0 or 1 */
} event_list_one[SERVERS_ONE + 1];



double GetArrivalBlockOne(void)
/* ---------------------------------------------
 * generate the next arrival time, with rate 1/2
 * ---------------------------------------------
 */
{
    static double arrival = START;

    SelectStream(0);
    arrival += Exponential(2.0);
    return (arrival);
}

double GetServiceBlockOne(void)
/* ---------------------------------------------
 * generate the next service time, with rate 1/6
 * ---------------------------------------------
 */
{
    SelectStream(1);
    return (Uniform(2, 10));
}

int NextEventBlockOne(event_list_one event)
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

    while (i < SERVERS_ONE)
    {        /* now, check the others to find which  */
        i++; /* event type is most imminent          */
        if ((event[i].x == 1) && (event[i].t < event[e].t))
            e = i;
    }

    return (e);
}

int FindOneBlockOne(event_list_one event)
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
    while (i < SERVERS_ONE)
    {        /* now, check the others to find which   */
        i++; /* has been idle longest                 */
        if ((event[i].x == 0) && (event[i].t < event[s].t))
            s = i;
    }
    return (s);
}

void *block1()
{

    stopFlag = 0;
    stopFlag2 = 0;

    struct
    {
        double current; /* current time                       */
        double next;    /* next (most imminent) event time    */
    } t;

    event_list_one event; /* The next-event list */

    long number = 0;   /* number in the node                 */
    int e;             /* next event index                   */
    int s;             /* server index                       */
    long index = 0;    /* used to count processed jobs       */
    double area = 0.0; /* time integrated number in the node */

    struct
    {                   /* accumulated sums of  */
        double service; /*   service times      */
        long served;    /*   number served      */
    } sum[SERVERS_ONE + 1];

    double dt = 0.0; /* departure time */
    float prob = 0;
    int forwarded = 0;


    PlantSeeds(123456789);

    /* Initialize arrival event */
    t.current = START;
    event[0].t = GetArrivalBlockOne();
    event[0].x = 1;

    /* Initialize server status */
    for (s = 1; s <= SERVERS_ONE; s++)
    {
        event[s].t = START; /* this value is arbitrary because */
        event[s].x = 0;     /* all servers are initially idle  */
        sum[s].service = 0.0;
        sum[s].served = 0;
    }

    struct sembuf oper;
    /* Unlock the orchestrator */
    oper.sem_num = 0;
    oper.sem_op = 1;
    oper.sem_flg = 0;
    semop(mainSem, &oper, 1);

    while ((event[0].x != 0) || (number != 0))
    {

        /* Wait for the start from the orchestrator */
        oper.sem_num = 0;
        oper.sem_op = -1;
        oper.sem_flg = 0;
        semop(sem, &oper, 1);

        printf("\n-------- BLOCK 1 --------\n");

        /* Find next event index */
        e = NextEventBlockOne(event);
        t.next = event[e].t;                   /* next event time  */
        area += (t.next - t.current) * number; /* update integral  */
        t.current = t.next;                    /* advance the clock*/

        //printf("\nCurrent time: %6.2f\n", t.current);

        if (e == 0)
        { /* Process an arrival */

            printf("\nBLOCK1: Processing an arrival...\n");
            printf("\tCurrent arrival time: %6.2f\n", event[0].t);

            /* Handle feedback_counter
            if (feedback_counter != 0) {
                number = number + feedback_counter;
                feedback_counter = 0;
            }
            */

            number++;

            event[0].t = GetArrivalBlockOne(); /* genera l'istante del prossimo arrivo */
            //printf("\tNext arrival: %6.2f\n", event[0].t);

            if (event[0].t > STOP) 
            {                   /* se si è arrivati alla fine non si avranno più arrivi */
                event[0].x = 0; /* verranno comunque completati i job rimanenti nel centro */
            }

            prob = get_forward_probability();
            printf("\nPROB: %f\n", prob);
            if (prob < THRESHOLD_PROBABILITY) { /* forwarded */
                printf("\nPROB: forward\n");
                forwarded++;
                number--;
                departureInfo.blockNum = 1;
                departureInfo.time = t.current;
                printf("\tForwarded departure: %6.2f\n", t.current);

            }else if (number <= SERVERS_ONE){
                /* se nel sistema ci sono al più tanti job quanti i server allora calcola un tempo di servizio */
                double service = GetServiceBlockOne();
                printf("\tService: %6.2f\n", service);
                s = FindOneBlockOne(event); /* trova un server vuoto */
                printf("\tServer selected: %d\n", s);
                sum[s].service += service;
                sum[s].served++;
                event[s].t = t.current + service; /* Aggiorna l'istante del prossimo evento su quel server (partenza) */
                event[s].x = 1;
            }

        }else{ /* Process a departure from server s */

            printf("\nBLOCK1: Processing a departure...\n");
            index++;
            number--; /* il job è stato completato */
            s = e;

            printf("\tDeparture: %6.2f\n", event[s].t);
            dt = event[s].t;
            if (number >= SERVERS_ONE)
            { /* se ci sono job in coda allora assegniamo un nuovo job
             con un nuovo tempo di servizio al
             server appena liberato */
                double service = GetServiceBlockOne();
                sum[s].service += service;
                sum[s].served++;
                event[s].t = t.current + service; /* calcola l'istante del
                                                     prossimo evento su quel server (partenza) */
            }
            else
            {
                event[s].x = 0; /* altrimenti quel server resta idle */
            }

            /* prepara il ritorno da dare all'orchestrator */
            /*
            glblDep = (block_queue *)malloc(sizeof(block_queue));
            if (glblDep == NULL)
            {
                perror("Malloc error:");
                return NULL;
            }
            
            glblDep->block = 1;
            glblDep->time = dt;
            */

           departureInfo.blockNum = 1;
           departureInfo.time = dt;
        }

        /* l' orchestrator deve sapere quale sarà il prossimo evento di questo blocco */
        e = NextEventBlockOne(event);                       /* next event index */
        update_next_event(1, event[e].t, (e == 0) ? 0 : 1); /* (e == 0) ? 0 : 1 significa che se e è uguale a 0 allora passa 0 (arrivo) altrimenti passa 1 (partenza) */

        printf("\n\tnumber: %ld arrival status: %d\n", number, event[0].x);

        printf("--------------------------\n\n");


        oper.sem_num = 0;
        oper.sem_op = 1;
        oper.sem_flg = 0;
        semop(mainSem, &oper, 1);
    }

    /* incrementa lo stop flag e metti il tuo next event time a infinito */
    whoIsFree[0] = 1;
    stopFlag = 1;
    update_next_event(1, INFINITY, -1);

    printf("BLOCK1: Terminated, waiting for the orchestrator...\n");
    /* attendi che l'orchestrator ti dia il via libera per stampare le statistiche*/
    oper.sem_num = 0;
    oper.sem_op = -1;
    oper.sem_flg = 0;
    semop(sem, &oper, 1);

    printf("\nBLOCK 1 STATISTICS:");
    printf("\n\nfor %ld jobs, forwarded %d\n", index, forwarded);
    printf("  avg interarrivals .. = %6.6f\n", event[0].t / index);
    printf("  avg wait ........... = %6.6f\n", area / index);
    printf("  avg # in node ...... = %6.6f\n", area / t.current);

    for (s = 1; s <= SERVERS_ONE; s++) /* adjust area to calculate */
        area -= sum[s].service;        /* averages for the queue   */

    printf("  avg delay .......... = %6.6f\n", area / index);
    printf("  avg # in queue ..... = %6.6f\n", area / t.current);
    printf("\nthe server statistics are:\n\n");
    printf("    server     utilization     avg service        share\n");
    for (s = 1; s <= SERVERS_ONE; s++)
        printf("%8d %14.3f %15.2f %15.3f\n", s, sum[s].service / t.current,
               sum[s].service / sum[s].served,
               (double)sum[s].served / index);

    printf("\n");

    pthread_exit((void *)0);
}
