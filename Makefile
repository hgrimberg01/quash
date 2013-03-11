all: quash

utility.o: src/utility.cpp
	clang++ -std=gnu++11 -c -g src/utility.cpp -o $@

quash.o: src/quash.cpp
	clang++ -std=gnu++11 -c -g src/quash.cpp -o $@
	
quash: utility.o quash.o
	clang++ -std=gnu++11 -g -o quash utility.o quash.o

test: quash
	./quash

clean:
	find . -name "*.o" | xargs rm
	find . -name "*~" | xargs rm
	rm quash
