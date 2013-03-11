all: quash

quash.o: src/quash.cpp
	g++ -O2 -std=c++11 -c -g src/quash.cpp -o $@
	
quash:  quash.o
	g++ -O2 -std=c++11 -g -o quash  quash.o

test: quash
	./quash

clean:
	find . -name "*.o" | xargs rm
	find . -name "*~" | xargs rm
	rm quash
