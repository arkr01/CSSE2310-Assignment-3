CFLAGS = -Wall -pedantic -g -lm -std=gnu99
.PHONY: all clean
.DEFAULT_GOAL := all

all: 2310A 2310B 2310dealer

2310dealer: 2310dealer.o 2310X.o dealerErrors.o
	gcc $(CFLAGS) -o 2310dealer 2310dealer.o 2310X.o playerErrors.o dealerErrors.o

2310B: 2310B.o 2310X.o playerErrors.o
	gcc $(CFLAGS) -o 2310B 2310B.o 2310X.o playerErrors.o

2310A: 2310A.o 2310X.o playerErrors.o
	gcc $(CFLAGS) -o 2310A 2310A.o 2310X.o playerErrors.o

2310dealer.o: 2310dealer.c 2310dealer.h
	gcc $(CFLAGS) -c 2310dealer.c

2310B.o: 2310B.c
	gcc $(CFLAGS) -c 2310B.c

2310A.o: 2310A.c
	gcc $(CFLAGS) -c 2310A.c

2310X.o: 2310X.c 2310X.h
	gcc $(CFLAGS) -c 2310X.c

dealerErrors.o: dealerErrors.c dealerErrors.h
	gcc $(CFLAGS) -c dealerErrors.c

playerErrors.o: playerErrors.c playerErrors.h
	gcc $(CFLAGS) -c playerErrors.c

clean:
	rm *.o 2310A 2310B 2310dealer
