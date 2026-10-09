CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -O2

.PHONY: all demo test sanitize clean
all: build/demo build/test_allocator

build:
	mkdir -p build

build/demo: allocator.c allocator.h demo.c | build
	$(CC) $(CFLAGS) allocator.c demo.c -o $@

build/test_allocator: allocator.c allocator.h test_allocator.c | build
	$(CC) $(CFLAGS) allocator.c test_allocator.c -o $@

demo: build/demo
	./build/demo

test: build/test_allocator
	./build/test_allocator

sanitize: | build
	$(CC) -std=c11 -Wall -Wextra -Wpedantic -Werror -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer allocator.c test_allocator.c -o build/test_sanitize
	./build/test_sanitize

clean:
	rm -rf build
