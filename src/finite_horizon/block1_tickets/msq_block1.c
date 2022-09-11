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
#include <sys/sem.h>
#include <math.h>
#include <pthread.h>

#include "../finite_helper.h"
#include "block1_helper.h"

#define START 0.0        /* initial (open the door)        */
#define MAX_SERVERS_ONE 200    /* number of servers              */
#define SERVERS_ONE_F1 20
#define SERVERS_ONE_F2 10
#define STOP 57600.0     /* terminal (close the door) time  (Fascia1: 36000, Fascia2: 21600)*/
#define CHANGE 36000.0
#define THRESHOLD_PROBABILITY 0.6        /* forward probability */

/* ricordiamoci che questi sono 1/lambda */
#define INT1 2.4    /* interrarivi (1/lambda1) */
#define INT2 4.32   /* interrarivi (1/lambda2) */
#define M1 120

FILE *fp;
int arrival;
int numberOfServers;

typedef struct
{             /* the next-event list    */
    double t; /*   next event time      */
    int x;    /*   event status, 0 or 1 */
} event_list_one[MAX_SERVERS_ONE + 1];


double GetArrivalBlockOneF1(void)
/* ---------------------------------------------
 * generate the next arrival time, with rate 1/2
 * ---------------------------------------------
 */
{
    SelectStream(0);
    arrival += Exponential(INT1);
    return (arrival);
}

double GetArrivalBlockOneF2(void)
/* ---------------------------------------------
 * generate the next arrival time, with rate 1/2
 * ---------------------------------------------
 */
{
    SelectStream(0);
    arrival += Exponential(INT2);
    return (arrival);
}

double GetServiceBlockOne(void)
/* ---------------------------------------------
 * generate the next service time, with rate 1/6
 * ---------------------------------------------
 */
{
    SelectStream(1);
    return (Exponential(M1));
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

    while (i < MAX_SERVERS_ONE)
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
    while (i < numberOfServers)
    {        /* now, check the others to find which   */
        i++; /* has been idle longest                 */
        if ((event[i].x == 0) && (event[i].t < event[s].t))
            s = i;
    }
    return (s);
}

void *block1()
{   
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

    double tmpArea = 0.0;

    struct
    {                   /* accumulated sums of  */
        double service; /*   service times      */
        long served;    /*   number served      */
    } sum[MAX_SERVERS_ONE + 1];

    double dt = 0.0; /* departure time */
    float prob = 0;
    int forwarded = 0;

    double lastArrival = 0.0;
    double totalService = 0.0;
    double totalUtilization = 0.0;

    stopFlag = 0;
    stopFlag2 = 0;

    arrival = START;

    int changeConfiguration = 0;
    int numberOfServers = SERVERS_ONE_F1;

    /* Initialize arrival event */
    t.current = START;
    event[0].t = GetArrivalBlockOneF1();
    event[0].x = 1;

    /* Initialize server status */
    for (s = 1; s <= MAX_SERVERS_ONE; s++)
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

        //printf("\n-------- BLOCK 1 --------\n");

        /* Find next event index */
        e = NextEventBlockOne(event);
        t.next = event[e].t;                   /* next event time  */
        //area += (t.next - t.current) * number; /* update integral  */
        tmpArea = (t.next - t.current) * number; /* update integral  */
        t.current = t.next;                    /* advance the clock*/

        glblWaitBlockOne = area / index;

        if (e == 0)
        { /* Process an arrival */

            number++;
            
            if (changeConfiguration == 0) {
                event[0].t = GetArrivalBlockOneF1(); /* genera l'istante del prossimo arrivo */
            }
            else {
                event[0].t = GetArrivalBlockOneF2(); /* genera l'istante del prossimo arrivo */
            }
            
            /* Controllo se l'arrivo è fascia 2, se è fascia imposta un flag
               per far si che dopo cerchi un server tra il nuovo numero */
            if (event[0].t > CHANGE) {
                changeConfiguration = 1;
                numberOfServers = SERVERS_ONE_F2;
            }

            if (event[0].t > STOP) 
            {                   /* se si è arrivati alla fine non si avranno più arrivi */
                event[0].x = 0; /* verranno comunque completati i job rimanenti nel centro */
            }

            prob = get_forward_probability();
            //printf("\nPROB: %f\n", prob);
            if (prob < THRESHOLD_PROBABILITY) { /* forwarded */
                //printf("\nPROB: forward\n");
                forwarded++;
                number--;
                departureInfo.blockNum = 1;
                departureInfo.time = t.current;
                //printf("\tForwarded departure: %6.2f\n", t.current);

            }else if (number <= numberOfServers){
                /* se nel sistema ci sono al più tanti job quanti i server allora calcola un tempo di servizio */
                lastArrival = event[0].t;
                double service = GetServiceBlockOne();
                //printf("\tService: %6.2f\n", service);
                s = FindOneBlockOne(event); /* trova un server vuoto */
                //printf("\tServer selected: %d\n", s);
                sum[s].service += service;
                sum[s].served++;
                event[s].t = t.current + service; /* Aggiorna l'istante del prossimo evento su quel server (partenza) */
                event[s].x = 1;

                area += tmpArea;

            }else{
                lastArrival = event[0].t;
                area += tmpArea;
            }

        }else{ /* Process a departure from server s */

            //printf("\nBLOCK1: Processing a departure...\n");
            index++;
            number--; /* il job è stato completato */
            s = e;

            area += tmpArea;

            //printf("\tDeparture: %6.2f\n", event[s].t);
            dt = event[s].t;
            if (number >= numberOfServers)
            { /* se ci sono job in coda allora assegniamo un nuovo job
             con un nuovo tempo di servizio al
             server appena liberato */


                /* Decrementare contatore di server da togliere 
                   Ogni volta controllo il contatore a zero
                   Se a zero, allora nulla
                   Se diverso, allora si decrementa e si salta all'else */
                if (s <= numberOfServers) { /* Possiamo assegnare il server */
                    double service = GetServiceBlockOne();
                    sum[s].service += service;
                    sum[s].served++;
                    event[s].t = t.current + service; /* calcola l'istante del
                                                        prossimo evento su quel server (partenza) */
                }
            }
            else
            {
                event[s].x = 0; /* altrimenti quel server resta idle */
            }

            /* prepara il ritorno da dare all'orchestrator */
            departureInfo.blockNum = 1;
            departureInfo.time = dt;
        }

        /* l' orchestrator deve sapere quale sarà il prossimo evento di questo blocco */
        e = NextEventBlockOne(event);                       /* next event index */
        update_next_event(1, event[e].t, (e == 0) ? 0 : 1); /* (e == 0) ? 0 : 1 significa che se e è uguale a 0 allora passa 0 (arrivo) altrimenti passa 1 (partenza) */

        //printf("\n\tnumber: %ld arrival status: %d\n", number, event[0].x);

        //printf("--------------------------\n\n");

        oper.sem_num = 0;
        oper.sem_op = 1;
        oper.sem_flg = 0;
        semop(mainSem, &oper, 1);
    }

    /* incrementa lo stop flag e metti il tuo next event time a infinito */
    whoIsFree[0] = 1;
    stopFlag = 1;
    update_next_event(1, INFINITY, -1);

    //printf("BLOCK1: Terminated, waiting for the orchestrator...\n");
    /* attendi che l'orchestrator ti dia il via libera per stampare le statistiche*/
    oper.sem_num = 0;
    oper.sem_op = -1;
    oper.sem_flg = 0;
    semop(sem, &oper, 1);

    printf("\nBLOCK 1 STATISTICS:");
    printf("\n\nfor %ld jobs, forwarded %d\n", index, forwarded);
    printf("  avg interarrivals .. = %6.6f\n", lastArrival / index);
    printf("  avg wait ........... = %6.6f\n", area / index);
    printf("  avg # in node ...... = %6.6f\n", area / t.current);

    /* Write statistics on file */
    /*
    fp = fopen(FILENAME_WAIT_BLOCK1, "a");
    fprintf(fp,"%6.6f\n", area / index);
    fclose(fp);
    */

    for (s = 1; s <= SERVERS_ONE_F1; s++) /* adjust area to calculate */
        area -= sum[s].service;        /* averages for the queue   */

    printf("  avg delay .......... = %6.6f\n", area / index);
    printf("  avg # in queue ..... = %6.6f\n", area / t.current);
    printf("\nthe server statistics are:\n\n");
    printf("    server     utilization     avg service        share\n");
    for (s = 1; s <= SERVERS_ONE_F1; s++) {
        printf("%8d %14.3f %15.2f %15.3f\n", s, sum[s].service / t.current,
               sum[s].service / sum[s].served,
               (double)sum[s].served / index);
        totalService += sum[s].service / sum[s].served;
        totalUtilization += sum[s].service / t.current;
    }
    
    //avgService = totalService / SERVERS_ONE;

    //printf("\n   avg service ........ = %6.6f\n", avgService / SERVERS_ONE);
    //printf("   avg utilization .... = %6.6f\n", totalUtilization / SERVERS_ONE);

    /* Write statistics on file */
    /*
    fp = fopen(FILENAME_DELAY_BLOCK1, "a");
    fprintf(fp,"%6.6f\n", area / index);
    fclose(fp);
    */

    printf("\n");
    
    pthread_exit((void *)0);
}
