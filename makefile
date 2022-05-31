all: encoder

encoder: encoder.o
	gcc encoder.o -lm -o encoder

encoder.o: encoder.c encoder.h
	gcc -c encoder.c


clean:
	rm -rf *.o
