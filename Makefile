CC = gcc
CFLAGS = -g -Wall
OBJS = myshell.o execute.o parser.o signals.o

all: myshell

myshell: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o myshell

myshell.o: myshell.c execute.h parser.h signals.h
	$(CC) $(CFLAGS) -c myshell.c

execute.o: execute.c execute.h
	$(CC) $(CFLAGS) -c execute.c

parser.o: parser.c parser.h
	$(CC) $(CFLAGS) -c parser.c

signals.o: signals.c signals.h
	$(CC) $(CFLAGS) -c signals.c

clean:
	rm -f $(OBJS) myshell