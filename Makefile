CC=gcc
CCFLAGS=-I.
LIBS=

all: quash

quash.o: src/quash.c
	$(CC) $(CCFLAGS) -c $@

quash: src/quash.o
	$(CC) -o $@ $^ $(CCFLAGS) $(LIBS)

test: quash
	./quash
	
clean:
	find . -name "*.o" | xargs rm
	find . -name "*~" | xargs rm
	rm quash
