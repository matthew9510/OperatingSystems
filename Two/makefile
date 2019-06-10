# The official makefile for Program 2, CS570

PROGRAM = p2
CC = gcc
CFLAGS = -g

${PROGRAM}:	${PROGRAM}.o getword.o
		${CC} ${PROGRAM}.o getword.o -o ${PROGRAM}

getword.o:	getword.h

${PROGRAM}.o:	${PROGRAM}.h getword.h

clean:
		rm -f *.o ${PROGRAM} your.output*

splint:
		splint -warnposix +trytorecover -weak getword.c ${PROGRAM}.c
