# A Makefile for Program 0, CS570

PROGRAM = p0
CC = gcc
CFLAGS = -g

${PROGRAM}:	getword.o ${PROGRAM}.o
		${CC} -o ${PROGRAM} getword.o ${PROGRAM}.o

${PROGRAM}.o:	getword.h

getword.o:	getword.h

clean:
		rm -f *.o ${PROGRAM}
