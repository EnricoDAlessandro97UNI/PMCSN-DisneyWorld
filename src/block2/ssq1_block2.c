
/* -------------------------------------------------------------------------
 * This program simulates a single-server FIFO service node using arrival
 * times and service times read from a text file.  The server is assumed
 * to be idle when the first job arrives.  All jobs are processed completely
 * so that the server is again idle at the end of the simulation.   The
 * output statistics are the average interarrival time, average service
 * time, the average delay in the queue, and the average wait in the service 
 * node. 
 *
 * Name              : ssq1.c  (Single Server Queue, version 1)
 * Authors           : Steve Park & Dave Geyer
 * Language          : ANSI C
 * Latest Revision   : 9-01-98
 * Compile with      : gcc ssq1.c 
 * ------------------------------------------------------------------------- 
 */

/*
  Questo programma legge da file gli arrivi. Quindi per prima devo modificarlo per far si che:
	- legge un arrivo e lo mette in una certa coda
	- ogni volta che il server si libera il prossimo job da eseguire è sempre quello nella
	  coda con priorità più alta, se c'è.
	- Ogni volta che viene preso in servizio un job della prima coda bisogna far si che ai tempi dei job nella coda
	  con priorità più bassa venga aggiunto il tempo di servizio di quel job.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <pthread.h>

#include "block2_helper.h"
#include "../rngs.h"
#include "../orchestrator_helper.h"

#define START      0.0
#define CLASS_THRESHOLD 5
#define STOP 200000.0

dep_list *dlisth = NULL;    /* head */
dep_list *dlistt = NULL;    /* cur */

queue *queue1h = NULL;    /* head */
queue *queue1t = NULL;    /* tail */
int nq1 = 0;    /* numero di job entrati in classe 1 */

queue *queue2h = NULL;    /* head */
queue *queue2t = NULL;    /* tail */
int nq2 = 0;    /* numero di job entrati in classe 2 */

double GetServiceBlockTwo(void)
/* ---------------------------------------------
 * generate the next service time, with rate 1/6
 * ---------------------------------------------
 */
{
    SelectStream(2);
    return (Exponential(2.0));
}

//int main() {
void *block2() {

    double garr, gserv;                            //global arrival, global service: servono a prendere un evento prima di verificare se vada in prima o seconda classe
    //per la coda ad alta priorità
    long index = 0;                              /* job index            */
    double arrival = START;                      /* arrival time         */
    double delay;                                /* delay in queue       */
    double service;                              /* service time         */
    double wait;                                 /* delay + service      */
    double departure = START;                    /* departure time       */
    double forcedwait = 0;                       /* forced wait due tue a class 2 job already in service */

    //per la coda con priorità più bassa
    long index2 = 0;                              /* job index            */
    double arrival2 = START;                      /* arrival time         */
    double delay2;                                /* delay in queue       */
    double service2;                              /* service time         */
    double wait2;                                 /* delay + service      */
    double departure2 = START;                    /* departure time       */
    double plus = 0;

    struct {                                       /* sum of ...           */
        double delay;                              /*   delay times        */
        double wait;                               /*   wait times         */
        double service;                            /*   service times      */
        double interarrival;                       /*   interarrival times */
    } sum = {0.0, 0.0, 0.0}, sum2 = {0.0, 0.0, 0.0};


    /*
    Ogni volta che viene letto un evento devo controllare in quale delle due code va, in base ad un certo valore chiamato CLASS_THRESHOLD.
    Dopodichè:
        - per la coda con priorità alta lascio così
        - per la coda con priorità bassa invece bisogna verificare se c'è un job della prima classe e in tal caso bisogna aggiungere all'attesa
        il tempo necessario che quello termini
    */

    int nextEvent;
    int jobs = 0;  /* Number of jobs in the node */

    double stop = STOP;

    int read_stopf = 1;

    struct sembuf oper;

    oper.sem_num = 0;
    oper.sem_op = 1;
    oper.sem_flg = 0;
    semop(mainSem, &oper, 1);

    while((stopFlag != 1) || (jobs != 0) || (arrivalsBlockTwo != NULL)) {
        
        oper.sem_num = 1;
        oper.sem_op = -1;
        oper.sem_flg = 0;
        semop(sem, &oper, 1);

        printf("\n-------- BLOCK 2 --------\n");

        nextEvent = get_next_event_type(2);  /* 0 arrival, 1 departure */
        if(nextEvent == 0) {   /* Process an arrival */

            jobs++;

            garr = GetArrivalFromQueue(2) -> time;
            DeleteFirstArrival(2);      /* delete event (arrival) from the arrivalsBlockTwo queue */
            gserv = GetServiceBlockTwo();

            printf("\tArrival block two: %6.2f\n", garr);
            printf("\tService block two: %6.2f\n", gserv);

            plus = 0;
            forcedwait = 0;

            if (gserv <= CLASS_THRESHOLD) {
                /* il job va in prima classe */
                index++;

                nq1++;

                arrival = garr;

                if (arrival < departure) {                    // se il tempo di arrivo del job è minore (precedente) alla partenza di quello
                    // in servizio allora avrà un ritardo
                    delay = departure - arrival;            /* delay in queue    */
                } else {                                        // se invece arriva dopo la partenza di quello in servizio non avrà alcun ritardo
                    delay = 0.0;
                    /* bisogna controllare che non ci sia un job di classe 2 in servizio. Se così fosse bisogna aspettare che finisca */
                    forcedwait = check_served(arrival);
                    /* no delay */
                }
                service = gserv;
                wait = delay +
                       service;            //il tempo di attesa in coda è dato da il ritardo accumulato + il servizio del job stess
                departure = arrival + wait + forcedwait;             /* time of departure */
                sum.delay += delay;
                sum.wait += wait;
                sum.service += service;

                queue *node = create_queue_node(arrival, service, departure, 1);
                queue_add(1, node);

                update_class2_depart(service,node);    /* se l'unico job di prima classe presente è in servizio i successvi di classe 2 non vedono questo job */

                //printf("job (1°)\narrival [%lf]\nservice [%lf]\ndeparture [%lf]\ntakes_serv [%lf]\nforced_wait [%lf]\n", arrival, service, departure, node->takes_service, forcedwait);

            } 
            else {
                /* il job è in seconda classe */

                index2++;

                nq2++;

                arrival2 = garr;

                if (arrival2 > departure) {            /* se l'arrivo nella seconda coda avviene dopo il completamento nella prima vuol dire che
                                                    non verrà rallentato */

                    if (arrival2 < departure2) {                    /* se il tempo di arrivo del job è minore (precedente) alla partenza di quello
                                                                    in servizio allora avrà un ritardo */

                        if (queue2t != NULL) {                        /* devo calcolare il delay in base alla partenza dell'ultimo in coda che potrebbe essere stato aggiornato
                                                        un caso sia arrivato qualche altro job di classe 1 */
                            delay2 = queue2t->departure - arrival2;            /* delay in queue    */
                        } else {
                            delay2 = departure2 - arrival2;
                        }

                    } else {                                        // se invece arriva dopo la partenza di quello in servizio non avrà alcun ritardo
                        delay2 = 0.0;                        /* no delay */
                    }

                    service2 = gserv;
                    wait2 = delay2 +
                            service2;                //il tempo di attesa in coda è dato da il ritardo accumulato + il servizio del job stess
                    departure2 = arrival2 + wait2;             /* time of departure */
                    sum2.delay += delay2;
                    sum2.wait += wait2;
                    sum2.service += service2;


                    queue *node = create_queue_node(arrival2, service2, departure2, 2);
                    queue_add(2, node);
                } 
                else {  /* In questo caso vuol dire che nella prima coda ci sono dei job e che quindi bisognerà attendere
                           non solo quelli della propria coda ma anche quelli della prima*/

                        /* tempo di attesa dovuto ai job in prima classe, dato dalla differenza fra il tempo di arrivo di tale job e il tempo
                           di partenza dell'ultimo job in prima classe. Questo però in realtà va fatto solo per il primo job che entra in coda della classe 2,
                           a tale proposito si controlla prima se il job è arrivato dopo l'ultima partenza.*/

                    //if (arrival2 < departure2) {   /* se il tempo di arrivo del job è minore (precedente) alla partenza di quello
                                                    //in servizio allora avrà un ritardo */
                    if(queue2t != NULL && arrival2 < queue2t->departure){
                        /* devo calcolare il delay in base alla partenza dell'ultimo in coda che potrebbe essere stato aggiornato
                           un caso sia arrivato qualche altro job di classe 1 */

                        /*
                              if(queue2t != NULL){
                                  delay2      = queue2t->departure - arrival2;

                              }else{

                                  delay2      = departure2 - arrival2;

                              }
                        */


                        if (queue2t -> takes_service < queue1t -> atime) {    /* se il job di classe 2 presente nel sistema è entrato in servizio prima dell'arrivo di uno di classe
                                                                    1 allora il ritardo dato da quest'ultimo deve essere agiunto al nuovo arrivo in classe 2 */
                            plus = departure - arrival2;            /* delay in queue */
                            //printf("#zzz departure %f  arrival %f\n", departure, arrival2);
                            delay2 = 0;
                        } else {
                            //printf("#zzz departure %f  arrival %f\n", queue2t->departure, arrival2);
                            delay2 = queue2t -> departure - arrival2;
                        }

                    } else {                                        // se invece arriva dopo la partenza di quello in servizio non avrà alcun ritardo
                        //printf("#www departure %f  arrival %f\n", departure2, arrival2);
                        delay2 = 0.0;                        /* no delay da quelli nella propria coda perchè non c'è nessuno*/
                        plus = departure - arrival2;            /* delay in queue */

                    }

                    service2 = gserv;
                    wait2 = delay2 + service2 + plus;        /*il tempo di attesa in coda è dato da il ritardo accumulato + il servizio del
                                                                    job stess e in questo caso bisogna aggiungere anche il tempo di attesa nella prima coda*/
                    departure2 = arrival2 + wait2;             /* time of departure */
                    sum2.delay += delay2;
                    sum2.wait += wait2;
                    sum2.service += service2;


                    queue *node = create_queue_node(arrival2, service2, departure2, 2);
                    queue_add(2, node);
                    //printf("job (2°)\narrival [%lf]\nservice [%lf]\ndeparture [%lf]\ntakes_serv [%lf]\n", arrival2, service2, departure2, node->takes_service);
                    //printf("delay: %lf wait: %lf plus: %lf\n", delay2, wait2, plus);
                }
            }
        } 
        else { /* Process a departure */

            jobs--;

            /* è una partenza: bisogna passarla all'orchestrator */
            /* prepara il ritorno da dare all'orchestrator */

            departureInfo.blockNum = 2;
            departureInfo.time = get_next_event_time(2);
            printf("\tBLOCK2: Departure %6.2f\n", departureInfo.time);

            /* bisogna eliminare la testa dalla coda di una delle due classi, da quella in cui si trovava il job che è partito*/
            delete_departed_event();
        }
        print_queue(queue1h);
        print_queue(queue2h);
        /* ora bisogna controllare quale sarà la prossima partenza per settare il prossimo evento */
        set_next_event(2);

        oper.sem_num = 0;
        oper.sem_op = 1;
        oper.sem_flg = 0;
        semop(mainSem, &oper, 1);

        if(stopFlag == 1 && read_stopf == 1){
            /* aggiorno il tempo di conclusione di questo blocco mettendo il tempo dell'ultimo job + 1 */
            stop = get_last_event_b2() + 1;
            printf("STOOOOOOOOOOOOOOOOOOOOOOOP %6.2f\n", stop);
            read_stopf = 0;
            //inf_f = -1; /* è impossibile che esca -1 e devo far si che prima di terminare la seconda condizione del while sia falsa */
        }

        printf("######################## JOBS %d ########################\n", jobs);
        printf("\t\t\t next_e %6.2f -  %d ########################\n", get_next_event_time(2), jobs);
    }

    stopFlag = 2;
    update_next_event(2, INFINITY, -1);

    /* attendi che l'orchestrator dia il via libera */
    oper.sem_num = 1;
    oper.sem_op = -1;
    oper.sem_flg = 0;
    semop(sem,&oper,1);

    printf("\nBLOCK 2 STATISTICS:\n");
    printf("First class:\n###############################################\n");

    sum.interarrival = arrival - START;

    printf("\nfor %ld jobs\n", index);
    printf("   average interarrival time = %6.2f\n", sum.interarrival / index);
    printf("   average service time .... = %6.2f\n", sum.service / index);
    printf("   average delay ........... = %6.2f\n", sum.delay / index);
    printf("   average wait ............ = %6.2f\n", sum.wait / index);
    printf("###############################################\n\n");

    printf("Second class:\n###############################################\n");

    sum2.interarrival = arrival2 - START;

    printf("\nfor %ld jobs\n", index2);
    printf("   average interarrival time = %6.2f\n", sum2.interarrival / index2);
    printf("   average service time .... = %6.2f\n", sum2.service / index2);
    printf("   average delay ........... = %6.2f\n", sum2.delay / index2);
    printf("   average wait ............ = %6.2f\n", sum2.wait / index2);
    printf("###############################################\n\n");

    pthread_exit((void *) 1);
}
