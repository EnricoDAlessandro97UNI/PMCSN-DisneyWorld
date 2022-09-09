#ifndef PROGETTOPMCSN_ORCHESTRATOR_HELPER_H
#define PROGETTOPMCSN_ORCHESTRATOR_HELPER_H

#include <stdio.h>
#include "rngs.h"

typedef struct next_event { 
    double time;    /* istante dell'evento */
    int eventType;     /* 0: arrivo 1: partenza */
} next_event;


typedef struct glbl_info {
    int blockNum;                /* Numero di blocco */
    int available_event_flag;   /* PER I BLOCCHI MSQ: serve per far si di gestire il caso in cui non siano
                                   * disponibili ancora due eventi per questo blocco. Importa solo per i blocchi
                                   * della 'lista di testa' */
    next_event *ne;             /* next event for this block */
    struct glbl_info *next;     /* coda di blocchi */
} glbl_info;

/* Structure used to handle global event times */
typedef struct global_info {
    double time;    /* Event time */
    int eventType;  /* Event type: 0 arrival in the block, 1 departure from the block */
} global_info;

/* Structure used to return the departure info to the orchestrator */
typedef struct departure_info {
    int blockNum;   /* Block from which there was a departure */
    double time;    /* Departure time */
} departure_info;

/* stuttura utilizzata dai blocchi per ritornare una partenza */
typedef struct block_queue {
    int block;
    double time;
    struct block_queue *next;
} block_queue;


/* ogni blocco che ha una partenza alloca spazio e ci mette dentro i valori prescelti.
 * L' orchestrator, dopo aver preso i dati fa la free. */
extern block_queue *glblDep;

extern glbl_info *glblHead;

/* -------------- GLOBAL VARIABLES -------------- */
extern global_info globalInfo[4];

extern block_queue *arrivalsBlockOne;
extern block_queue *arrivalsBlockTwo;
extern block_queue *arrivalsBlockThree;
extern block_queue *arrivalsBlockFour;
extern block_queue *arrivalsBlockFive;

extern departure_info departureInfo;

extern int sem;
extern int mainSem;

extern int stopFlag;
/* ---------------------------------------------- */

extern int feedback_counter;

double Exponential(double m);
double Uniform(double a, double b);

void create_list();
void init_list();
void init_global_info_structure();
int get_next_event();
int get_next_event_type(int);
double get_next_event_time(int);
int add_event_to_queue(double time, int block);
void update_next_event(int, double, int);
void update_availability(int block);
glbl_info* GetArrivalFromList(int block);
double GetServiceFromList(int block);
double GetService(void);
int GetSkateType();
void init_queues();
block_queue* GetArrivalFromQueue(int block);
int DeleteFirstArrival(int block);
int get_available_e_flag(int block);
void update_next_event_after_arrival(int block, double time);
int set_available_e_flag(int block);

#endif 