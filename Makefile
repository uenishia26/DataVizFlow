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
	rm -f $(OBJS) get_test.o tappet.o buffer.o observe.o reconstruct.o tapplot.o tappet libobjdata.so myshell

get_test.o: get_test.c
	gcc -fPIC -c get_test.c -o get_test.o
buffer.o: buffer.c
	gcc -fPIC -c buffer.c -o buffer.o
observe.o: observe.c
	gcc -fPIC -c observe.c -o observe.o
reconstruct.o: reconstruct.c
	gcc -fPIC -c reconstruct.c -o reconstruct.o
tapplot.o:
	gcc -fPIC -c tapplot.c -o tapplot.o
share_lib: get_test.o tappet.o buffer.o observe.o reconstruct.o tapplot.o
	gcc -shared -o libobjdata.so get_test.o tappet.o buffer.o observe.o reconstruct.o tapplot.o -pthread
tappet.o: tappet.c
	gcc -fPIC -c tappet.c -o tappet.o
tappet: tappet.o share_lib
	gcc $(CFLAGS) tappet.o -L. -o tappet -pthread -ldl
