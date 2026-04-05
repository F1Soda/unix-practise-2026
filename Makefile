.PHONY: all clean main

all: main worker

main: src/main.c src/lock.c
	mkdir -p bin
	gcc -g src/main.c src/lock.c -o bin/main

worker: src/worker.c src/lock.c
	mkdir -p bin
	gcc -g src/worker.c src/lock.c -o bin/worker

clean:
	rm -rf out bin
