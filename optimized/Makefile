CC = gcc
CFLAGS = -O2 -Wall -I .
FFLAGS = -rdynamic

# This flag includes the Pthreads library on a Linux box.
# Others systems will probably require something different.
LIB = -lpthread -ldl

all: optimized adder

optimized: optimized.c csapp.o
	$(CC) $(CFLAGS) $(FFLAGS) -o optimized optimized.c csapp.o $(LIB)

csapp.o:
	$(CC) $(CFLAGS) -c csapp.c

adder:
	$(CC) -shared -fPIC -o libadder.so adder.c csapp.c

clean:
	rm -f *.o optimized  *~ libadder.so
