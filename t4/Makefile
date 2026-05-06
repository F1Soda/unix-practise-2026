.PHONY: all clean server client

all: server client number_generator

server: src/server/logger.c src/server/main.c src/server/server.c
	mkdir -p bin
	gcc -g src/server/logger.c src/server/main.c src/server/server.c -o bin/server

client: src/client/main.c
	mkdir -p bin
	gcc -g src/client/main.c -o bin/client

number_generator: src/number_generator/main.c
	mkdir -p bin
	gcc -g src/number_generator/main.c -o bin/number_generator


clean:
	rm -rf bin