//
// Created by alessandrodea on 20/08/22.
//

#include <stdio.h>
#include <stdlib.h>
#include "block2_helper.h"
#include "../orchestrator_helper.h"
#include <math.h>

/* crea un nodo per una qualsiasi delle due queue */
queue* create_queue_node(double at, double st, double dt, int c){
    queue* node = (queue*)malloc(sizeof(queue));
    if(node == NULL){
        perror("Malloc error:");
        return NULL;
    }

    node->class = c;
    node->atime = at;
    node->stime = st;
    node->departure = dt;
    node->takes_service = dt - st;	/* questo è l'istante in cui il job prende servizio. Serve a far si che quando arriva un job di classe 1
									se un job di classe 2 è gia entrato in servizio allora a lui non viene aumentato il tempo di attesa. Se così non si facesse
									allora è come se stessimo implementando la PREEMPTION */
    return node;
}

/* crea un nodo per la lista delle partenze */
dep_list* create_list_node(double dt, double st, int c){
    dep_list* node = (dep_list*)malloc(sizeof(dep_list));
    if(node == NULL){
        perror("Malloc error:");
        return NULL;
    }

    node->class = c;
    node->dtime = dt;
    node->stime = st;
    return node;
}

void set_next_event(int block){

    double min1 = INFINITY;
    double min2 = INFINITY;
    if(queue1h != NULL) {
        queue *h1 = queue1h;
        min1 = h1->departure;
        /* controlla la partenza più imminente della prima classe */
        while (h1 != NULL) {
            if (h1->departure < min1) {
                min1 = h1->departure;
            }
            h1 = h1->next;
        }
    }
    if(queue2h != NULL) {
        queue *h2 = queue2h;
        min2 = h2->departure;
        /* controlla la partenza più imminente della seconda classe */
        while (h2 != NULL) {
            if (h2->departure < min2) {
                min2 = h2->departure;
            }
            h2 = h2->next;
        }
    }

    globalInfo[block-1].time = (min1 <= min2) ? min1 : min2;
    globalInfo[block-1].eventType = 1;

    /*
    glbl_info *tmp = glbl_head;
    int c = 1;
    while (c != block){
        c++;
        tmp = tmp->next;
    }

    tmp->ne->time = (min1 <= min2) ? min1 : min2;
    tmp->ne->e_type = 1;
    */
}


/* aggiungi un nodo alla queue specificata tramite la sua head */
void queue_add(int class, queue *node){

    if(class == 1){

        node->next = NULL;

        if(queue1h == NULL){

            node->prev = NULL;
            queue1h = node;
            queue1t = queue1h;

        }else{

            queue1t->next = node;
            node->prev = queue1t;
            queue1t = node;

        }

    }else if(class == 2){

        node->next = NULL;

        if(queue2h == NULL){

            node->prev = NULL;
            queue2h = node;
            queue2t = queue2h;

        }else{

            queue2t->next = node;
            node->prev = queue2t;
            queue2t = node;

        }

    }else{
        printf("INVALID CLASS: %d\n", class);
        exit(-1);
    }

}


/* elimina l'elemento in testa alla lista specificata tramite la sua head */
void queue_rm(int class){

    if(class == 1){
        if(queue1h->next != NULL) {
            queue *tmp = queue1h;
            queue1h = queue1h->next;
            queue1h->prev = NULL;
            free(tmp);
            tmp = NULL;
        }else{
            free(queue1h);
            queue1h = NULL;
        }
    }else{
        if(queue2h->next != NULL) {
            queue *tmp = queue2h;
            queue2h = queue2h->next;
            queue2h->prev = NULL;
            free(tmp);
            tmp = NULL;
        }else{
            free(queue2h);
            queue2h = NULL;
        }
    }

}


void delete_departed_event(){
    if(queue1h != NULL && queue2h != NULL){
        (queue1h->departure <= queue2h->departure) ? queue_rm(1) : queue_rm(2);
    } else{
        (queue1h == NULL) ? queue_rm(2) : queue_rm(1);

    }

}


/* aggiungi una nuova partenza alla lista */
void list_add(dep_list *node){
    node->next = NULL;

    if(dlisth == NULL){

        node->prev = NULL;
        dlisth = node;
        dlistt = dlisth;

    }else{

        dlistt->next = node;
        node->prev = dlistt;
        dlistt = node;

    }
}


/* tale funzione viene chiamata ogni qual volta che c'è una nuova partenza */
int queue_depart(queue *head, double dept){

    list_add(create_list_node(dept, head->stime, 0));	/* inserisce le informazioni necessarie della partenza nella lista delle partenze */
    queue_rm(head->class);		/* rimuove l'elemento dalla coda */

    return 0;
}


/* questa funzione aggiorna i tempi di partenza per i job di classe 2 nel momento in cui arriva un nuovo job in classe 1. Viene aumentato
l'istante di partenza del valore di tempo necessario per cui il job di classe 1 appena entrato verrà servito */
void update_class2_depart(double wait, queue *n){
    queue *tmp = queue2h;
    while(tmp != NULL){
        if(tmp->takes_service >= n->atime){
            tmp->departure += wait;
            tmp->takes_service += wait;
        }

        tmp = tmp->next;
    }
}



/* crea una lista di partenze ordinata facendo il merge delle due code */
void create_glbl_list(){

    queue *tmp1 = queue1h;
    queue *tmp2 = queue2h;

    while(tmp1 != NULL && tmp2 != NULL){

        if(tmp1->departure <= tmp2->departure){
            /* aggiungi alla lista globale il job di classe 1 */
            dep_list *node = create_list_node(tmp1->departure, tmp1->stime, 1);
            list_add(node);
            tmp1 = tmp1->next;
            //queue_rm(queue1h);
        }else{
            /* aggiungi alla lista globale il job di classe2 */
            dep_list *node = create_list_node(tmp2->departure, tmp2->stime, 2);
            list_add(node);
            tmp2 = tmp2->next;
            //queue_rm(queue2h);
        }
    }
    if(tmp1 == NULL){
        /* significa che sono rimasti da copiare alcuni job di classe 2 */
        while(tmp2 != NULL){
            dep_list *node = create_list_node(tmp2->departure, tmp2->stime, 2);
            list_add(node);
            tmp2 = tmp2->next;
        }

    }else{
        /* significa che sono rimasti da copiare alcuni job di classe 1 */
        while(tmp1 != NULL){
            dep_list *node = create_list_node(tmp1->departure, tmp1->stime, 1);
            list_add(node);
            tmp1 = tmp1->next;
        }
    }

}



void print_glbl_list(){
    dep_list *el = dlisth;
    while(el != NULL){
        printf("[d:%lf s:%lf (%d)]->", el->dtime, el->stime, el->class);
        el = el->next;
    }
    printf("\n");
    fflush(stdout);
}


void print_queue(queue *h){
    queue *t = h;
    while(t != NULL){
        printf("[d:%lf s:%lf a:%lf (%d)]->", t->departure, t->stime, t->atime, t->class);
        t = t->next;
    }
    printf("\n");
}



double check_served(double a){
    queue *t = queue2h;
    if(t == NULL)
        return 0;
    if(a > t->takes_service && a < t->departure)
        return (t->departure - a);

    return 0;
}

int print_on_file(){
    /*
    FILE* fp;
    fp = fopen(OUTPUTFILE, "w+");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open input file %s\n", OUTPUTFILE);
        return (1);
    }
    dep_list *el = dlisth;
    while(el != NULL){

        fprintf(fp, "%lf %lf\n", el->dtime, el->stime);

        el = el->next;

        fflush(fp);
    }

    fclose(fp);
    return 0;
    */
   return 0;
}

double get_last_event_b2(){

    double max1 = 0;
    double max2 = 0;
    if(queue1h != NULL) {
        queue *h1 = queue1h;
        max1 = h1->departure;
        /* controlla la partenza più imminente della prima classe */
        while (h1 != NULL) {
            if (h1->departure > max1 && h1->departure != INFINITY) {
                max1 = h1->departure;
            }
            h1 = h1->next;
        }
    }
    if(queue2h != NULL) {
        queue *h2 = queue2h;
        max2 = h2->departure;
        /* controlla la partenza più imminente della seconda classe */
        while (h2 != NULL) {
            if (h2->departure > max2 && h2->departure != INFINITY) {
                max2 = h2->departure;
            }
            h2 = h2->next;
        }
    }


    return (max1 > max2) ? max1 : max2;
}