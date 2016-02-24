CC=gcc
CFLAGS=-O2 -Wall -pthread

default: mmapbench
forkbench: mmapbench.o
	$(CC) -o $@ $^ $(CFLAGS)

run:
	python runtest.py
	./gnuplot.pg > mmap.png

clean:
	rm -rf *.o
	rm -rf mmapbench
	rm -rf *.png
