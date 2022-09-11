#ifndef PROGETTOPMCSN_ORCHESTRATOR_HELPER_H
#define PROGETTOPMCSN_ORCHESTRATOR_HELPER_H

#include <stdio.h>
#include "rngs.h"

#define SEED 123456789

#define FILENAME_WAIT_BLOCK1 "block1_tickets/wait_block1.dat"
#define FILENAME_DELAY_BLOCK1 "block1_tickets/delay_block1.dat"
#define FILENAME_WAIT_BLOCK2 "block2_dis/wait_block2.dat"
#define FILENAME_DELAY_BLOCK2 "block2_dis/delay_block2.dat"
#define FILENAME_WAIT_BLOCK3 "block3_norm/wait_block3.dat"
#define FILENAME_DELAY_BLOCK3 "block3_norm/delay_block3.dat"
#define FILENAME_WAIT_BLOCK4 "block4_contr/wait_block4.dat"
#define FILENAME_DELAY_BLOCK4 "block4_contr/delay_block4.dat"
#define FILENAME_WAIT_BLOCK5 "block5_storage/wait_block5.dat"
#define FILENAME_DELAY_BLOCK5 "block5_storage/delay_block5.dat"

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

/* -------------- GLOBAL VARIABLES -------------- */
extern global_info globalInfo[5];

extern block_queue *arrivalsBlockOne;
extern block_queue *arrivalsBlockTwo;
extern block_queue *arrivalsBlockThree;
extern block_queue *arrivalsBlockFour;
extern block_queue *arrivalsBlockFive;

extern departure_info departureInfo;

extern int sem;
extern int mainSem;

extern int stopFlag;
extern int stopFlag2;

extern int block4Lost;
extern int block4ToExit;

extern int whoIsFree[5];

/* ---------------------------------------------- */

double Exponential(double m);
double Uniform(double a, double b);
void create_statistics_files();
void init_global_info_structure();
int get_next_event();
int get_next_event_type(int);
double get_next_event_time(int);
int add_event_to_queue(double time, int block);
void update_next_event(int, double, int);
glbl_info* GetArrivalFromList(int block);
double GetServiceFromList(int block);
double GetService(void);
int GetSkateType();
void init_queues();
block_queue* GetArrivalFromQueue(int block);
int DeleteFirstArrival(int block);
void update_next_event_after_arrival(int block, double time);
float get_probability();
double Min(double a, double c);
float get_forward_probability();
double Erlang(long n, double b);
int unlock_waiting_threads();

#endif 