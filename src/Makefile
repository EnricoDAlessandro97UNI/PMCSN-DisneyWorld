# January 2, 2005
#
# Makefile for compiling accompanying software for
# "Discrete-Event Simulation: A First Course" by Park and Leemis
#

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lm -lpthread

OBJFILES = block1 block2 block3 block4 block5 simu rgns.o simul

all: $(OBJFILES)

simul:  orchestrator.c orchestrator_helper.c block1/msq_block1.c block2/ssq1_block2.c block2/block2_helper.c block3/msq_block3.c block4/msq_block4.c block4/block4_helper.c block5/msq_block5.c rngs.o 
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

block1: block1/msq_block1.c rngs.o
	$(CC) $^ -o $@ $(LDFLAGS)

block2: block2/ssq1_block2.c
	$(CC) $^ -o $@ -g $(LDFLAGS)

block3: block4/msq_block4.c rngs.o
	$(CC) $^ -o $@ $(LDFLAGS)

block4: block3/msq_block3.c block3_hrngs.o
	$(CC) $^ -o $@ $(LDFLAGS)

rngs.o: rngs.c rngs.h
	$(CC) -c $<


