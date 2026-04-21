.PHONY: all clean sparser fake-sparse-file-generator

all: sparser fake-sparse-file-generator

sparser: src/sparser.c
	mkdir -p bin
	gcc -g src/sparser.c -o bin/sparser

fake-sparse-file-generator: src/fake_sparse_file_generator.c
	mkdir -p bin
	gcc -g src/fake_sparse_file_generator.c -o bin/fake-sparse-file-generator

clean:
	rm -rf out bin
