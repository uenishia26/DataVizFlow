CC = gcc
CFLAGS = -g -Wall
OBJS = myshell.o execute.o parser.o signals.o

all: myshell tapper write read observe reconstruct tapplot

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
	rm -f -f *.o tapper write read observe reconstruct tapplot observe_t reconstruct_t tapplot_t dataFile.txt tappet libobjdata.so myshell

get_test.o: get_test.c
	gcc -fPIC -c get_test.c -o get_test.o
buffer.o: buffer.c
	gcc -fPIC -c buffer.c -o buffer.o
observe_t.o: observe_t.c
	gcc -fPIC -c observe_t.c -o observe_t.o
reconstruct_t.o: reconstruct_t.c
	gcc -fPIC -c reconstruct_t.c -o reconstruct_t.o
tapplot_t.o: tapplot_t.c
	gcc -fPIC -c tapplot_t.c -o tapplot_t.o
share_lib: get_test.o tappet.o buffer.o observe_t.o reconstruct_t.o tapplot_t.o
	gcc -shared -o libobjdata.so get_test.o tappet.o buffer.o observe_t.o reconstruct_t.o tapplot_t.o -pthread
tappet.o: tappet.c
	gcc -fPIC -c tappet.c -o tappet.o
tappet: tappet.o share_lib
	gcc $(CFLAGS) tappet.o -L. -o tappet -pthread -ldl

tapper: tapper.o
	gcc -o tapper tapper.o -pthread
write: write.o
	gcc -o write write.o -pthread
observe: observe.o
	gcc -o observe observe.o -pthread
reconstruct: reconstruct.o
	gcc -o reconstruct reconstruct.o -pthread
tapplot: tapplot.o
	gcc -o tapplot tapplot.o -pthread
tapper.o: tapper.c
	gcc -c tapper.c 
write.o: write.c
	gcc -c write.c
observe.o: observe.c
	gcc -c observe.c 
reconstruct.o: reconstruct.c
	gcc -c reconstruct.c
tapplot.o: tapplot.c
	gcc -c tapplot.c
