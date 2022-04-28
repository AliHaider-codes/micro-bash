uBash: uBash.o
	gcc -o uBash uBash.o

uBash.o: uBash.c
	gcc -c uBash.c
clean:
	rm -f *.o uBash
.PHONY: clean