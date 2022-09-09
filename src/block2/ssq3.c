/* ------------------------------------------------------------------------- 
 * This program is a next-event simulation of a single-server FIFO service
 * node using Exponentially distributed interarrival times and Exponentially 
 * distributed service times (i.e., a M/M/1 queue).  The service node is 
 * assumed to be initially idle, no arrivals are permitted after the 
 * terminal time STOP, and the service node is then purged by processing any 
 * remaining jobs in the service node.
 *
 * Name            : ssq3.c  (Single Server Queue, version 3)
 * Author          : Steve Park & Dave Geyer
 * Language        : ANSI C
 * Latest Revision : 10-19-98
 * ------------------------------------------------------------------------- 
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <pthread.h>

#include "../rngs.h"
#include "../orchestrator_helper.h"
#include "block2_helper.h"

#define START 0.0  /* initial time */

   double Min(double a, double b)
/* ------------------------------
 * return the smaller of a, b
 * ------------------------------
 */
{ 
  if (a < b)
    return (a);
  else
    return (b);
} 

   double GetServiceBlockTwo()
/* --------------------------------------------
 * generate the next service time with rate 2/3
 * --------------------------------------------
 */ 
{
  SelectStream(2);
  return (Exponential(1.0));
}  

  //int main(void)
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
  } area      = {0.0, 0.0, 0.0};
  long index  = 0;                  /* used to count departed jobs         */
  long number = 0;                  /* number of jobs in the node          */

  /*
  PlantSeeds(SEED);
  t.current    = START;           // set the clock                         
  t.arrival    = GetArrival();    // schedule the first arrival            
  t.completion = INFINITY;        // the first event can't be a completion 
  */

  int nextEvent;
  t.current = START;

  struct sembuf oper;
  oper.sem_num = 0;
  oper.sem_op = 1;
  oper.sem_flg = 0;
  semop(mainSem, &oper, 1);

  while ((stopFlag != 1) || (number > 0) || (arrivalsBlockTwo != NULL)) {

    oper.sem_num = 1;
    oper.sem_op = -1;
    oper.sem_flg = 0;
    semop(sem, &oper, 1);

    printf("\n-------- BLOCK 2 --------\n");

    nextEvent = get_next_event_type(2);   /* 0 arrival, 1 departure */
    t.next = get_next_event_time(2);      

    if (number > 0)  {                               /* update integrals  */
      area.node    += (t.next - t.current) * number;
      area.queue   += (t.next - t.current) * (number - 1);
      area.service += (t.next - t.current);
    }
    t.current       = t.next;                    /* advance the clock */

    if (nextEvent == 0) { /* Process an arrival */

      printf("\nBLOCK2: Processing an arrival...\n");

      number++;
      t.arrival = GetArrivalFromQueue(2) -> time;
      printf("\nArrival time: %6.2f\n", t.arrival);
      DeleteFirstArrival(2);      /* delete event (arrival) from the arrivalsBlockTwo queue */
      if (arrivalsBlockTwo == NULL) {
        printf("\nArrivalsBlockTwo empty!\n");
        t.arrival = INFINITY;
      }
      else {
        t.arrival = arrivalsBlockTwo -> time;
      }
      if ((stopFlag == 1) && (arrivalsBlockTwo == NULL)) {
        printf("\nI'm here\n");
        t.last = t.current;
        t.arrival = INFINITY;
      }
      if (number == 1) {
        t.completion = t.current + GetServiceBlockTwo();
      }
    }
    else { /* Process a departure */

      printf("\nBLOCK2: Processing a departure...\n");

      index++;
      number--;
      if (number > 0) {
        t.completion = t.current + GetServiceBlockTwo();
      }
      else {
        t.completion = INFINITY;
      }

      /* Return departure info to the orchestrator */
      departureInfo.blockNum = 2;
      departureInfo.time = get_next_event_time(2);
      printf("\tBLOCK2: Departure %6.2f\n", departureInfo.time);
    }

    //t.next          = Min(t.arrival, t.completion);  /* next event time   */
    /*
    if (number > 0)  {                               // update integrals  
      area.node    += (t.next - t.current) * number;
      area.queue   += (t.next - t.current) * (number - 1);
      area.service += (t.next - t.current);
    }
    t.current       = t.next;                    // advance the clock 
    */

    /*
    if (t.current == t.arrival)  {               // process an arrival 
      number++;
      t.arrival     = GetArrival();
      if (t.arrival > STOP)  {
        t.last      = t.current;
        t.arrival   = INFINITY;
      }
      if (number == 1)
        t.completion = t.current + GetService();
    }
    else {                                        // process a completion 
      index++;
      number--;
      if (number > 0)
        t.completion = t.current + GetService();
      else
        t.completion = INFINITY;
    }
    */

    /* Set next event for this block */
    if (Min(t.arrival, t.completion) == t.arrival) {
      update_next_event(2, t.arrival, 0);
    } 
    else {
      update_next_event(2, t.completion, 1);
    }

    printf("--------------------------\n\n");

    oper.sem_num = 0;
    oper.sem_op = 1;
    oper.sem_flg = 0;
    semop(mainSem, &oper, 1);
  } 

  stopFlag = 2;
  update_next_event(2, INFINITY, -1);
  printf("\nBLOCK2: Terminated, waiting for the orchestrator...\n");

  oper.sem_num = 1;
  oper.sem_op = -1;
  oper.sem_flg = 0;
  semop(sem,&oper,1);

  printf("\nBLOCK2 STATISTICS:\n");

  printf("\nfor %ld jobs\n", index);
  printf("   average interarrival time = %6.2f\n", t.last / index);
  printf("   average wait ............ = %6.2f\n", area.node / index);
  printf("   average delay ........... = %6.2f\n", area.queue / index);
  printf("   average service time .... = %6.2f\n", area.service / index);
  printf("   average # in the node ... = %6.2f\n", area.node / t.current);
  printf("   average # in the queue .. = %6.2f\n", area.queue / t.current);
  printf("   utilization ............. = %6.2f\n", area.service / t.current);

  pthread_exit((void *)0);
}
