CC=gcc

all: ttymz80

ttymz80: ttymz80.o mz80cmt.o z80.o
	$(CC) -o $@ $^

clean:
	-rm -f *.o mz
