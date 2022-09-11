//
// Created by alessandrodea on 20/08/22.
//

#ifndef PROGETTOPMCSN_HELPER_BLOCK2_H
#define PROGETTOPMCSN_HELPER_BLOCK2_H

#endif

#include <stdio.h>
#include <stdlib.h>


// questa è una semplice lista delle partenze dal nodo
typedef struct dep_list
{
    double dtime;			
    double stime;			
    int class; 				
    struct dep_list *next;
    struct dep_list *prev;
}dep_list;


typedef struct queue
{
    int class;         
    double atime;     
    double stime;			
    double departure;	
    double takes_service;
    struct queue *next;
    struct queue *prev;
}queue;

extern dep_list *dlisth;	
extern dep_list *dlistt;	

extern queue *queue1h;	
extern queue *queue1t;	
extern int nq1;	

extern queue *queue2h;	
extern queue *queue2t;	
extern int nq2;	

queue* create_queue_node(double at, double st, double dt, int c);
dep_list* create_list_node(double dt, double st, int c);
void queue_add(int class, queue *node);
void queue_rm(int class);
void list_add(dep_list *node);
int queue_depart(queue *head, double dept);
void update_class2_depart(double wait, queue *n);
void create_glbl_list();
void print_glbl_list();
void print_queue(queue *h);
double check_served(double a);
int print_on_file();

void set_next_event(int block);

void delete_departed_event();
double get_last_event_b2();

void *block2();