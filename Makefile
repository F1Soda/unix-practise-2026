.PHONY: all clean sparser fake-sparse-file-generator

all: sparser fake-sparse-file-generator

sparser: src/sparser.c src/file_utils.c
	mkdir -p bin
	gcc -g src/sparser.c src/file_utils.c -o bin/sparser

fake-sparse-file-generator: src/fake_sparse_file_generator.c src/file_utils.c
	mkdir -p bin
	gcc -g src/fake_sparse_file_generator.c src/file_utils.c -o bin/fake-sparse-file-generator

clean:
	rm -rf out bin
