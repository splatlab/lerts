CC=gcc -std=gnu11
#CFLAGS=-Wall -I. -g -pthread -Wno-unused-result
CFLAGS=-Wall -I. -O3 -pthread -Wno-unused-result
#CFLAGS=-Wall -I. -O3 -pthread -Wno-unused-result -DLOG_WAIT_TIME
#CFLAGS=-Wall -I. -g -pthread -Wno-unused-result -DLOG_WAIT_TIME
LDFLAGS= -lcrypto -lm -lpthread

ALL: bm

gqf.o: gqf.c gqf.h
cbm_zipf.o: cbm_zipf.c zipf.h
zipf.o: zipf.h zipf.c

bm.o: bm.c gqf.h zipf.h
main.o: main.c gqf.h

zipf:	zipf.o
	$(CC) $(CFLAGS) -o zipf zipf.o $(LDLIBS)

cbm_zipf:	zipf.o gqf.o cbm_zipf.o
	$(CC) $(CFLAGS) zipf.o gqf.o cbm_zipf.o $(LDFLAGS) -o cbm_zipf

bm: bm.o gqf.o zipf.o
	$(CC) $(CFLAGS) gqf.o zipf.o bm.o $(LDFLAGS) -o bm

main: main.o gqf.o
	$(CC) $(CFLAGS) gqf.o main.o $(LDFLAGS) -o main

clean:
	rm -f *.o bm cbm_zipf main core
