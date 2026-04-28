.PHONY: all clean myinit dummy

all: myinit dummy

myinit: src/myinit/main.c src/myinit/config_parser.c src/myinit/demonize.c src/myinit/logger.c src/myinit/myinit.c src/myinit/validation.c src/myinit/crash_handler.c
	mkdir -p bin
	gcc src/myinit/main.c src/myinit/config_parser.c src/myinit/demonize.c src/myinit/logger.c src/myinit/myinit.c src/myinit/validation.c src/myinit/crash_handler.c -o bin/myinit

dummy: src/dummy/dummy.c
	mkdir -p bin
	gcc src/dummy/dummy.c -o bin/dummy


clean:
	rm -rf out bin config.txt myinit.log tail_copy.txt
	cat /dev/null > out_myinit_dummy.txt