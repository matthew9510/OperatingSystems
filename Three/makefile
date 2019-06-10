# Makefile for Program 3 (Joggers and Shooters) CS570

CC = gcc
CFLAGS = -g -o -Wall -Wpointer-arith -Wcast-qual -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs -D_REENTRANT

# The first target in this makefile is 'test', so 'make' will run the test
# of your p3 with parameter '6'.  Since p3 is listed as a dependency, the test
# will first make sure p3 is up to date before running the test.
# If you only want to compile rather than run the test, use 'make p3' .
# You can try a test with a different seed (11) with 'make test2', and of
# course, you can [after compiling] just run p3 with whatever seed you wish.

test:	p3
	./p3 6

p3:	p3main.o p3helper.o
	${CC} -o p3 p3main.o p3helper.o -lpthread -pthread

p3main.o:	p3.h

p3helper.o:	p3.h

clean:
	rm -f p3 p3main.o p3helper.o 

test2:	p3
	./p3 11
