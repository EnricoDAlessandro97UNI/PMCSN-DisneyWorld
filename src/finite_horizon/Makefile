# January 2, 2005
#
# Makefile for compiling accompanying software for
# "Discrete-Event Simulation: A First Course" by Park and Leemis
#

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lm -lpthread

OBJFILES = block1 block2 block3 block4 block5 simu rgns.o simul batch finite

all: $(OBJFILES)

finite: finite.c finite_helper.c block1_tickets/msq_block1.c block2_dis/ssq_block2.c block3_norm/msq_block3.c block4_contr/msq_block4.c block4_contr/block4_helper.c block5_storage/msq_block5.c rngs.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

rngs.o: rngs.c rngs.h
	$(CC) -c $<


