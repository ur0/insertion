insertion: inject_library.o insertion.c util.h ptrace.h
	$(CC) $(CFLAGS) $@.c inject_library.o $(LDFLAGS) -o $@

inject_library.o:
	nasm -f elf64 inject_library.s

test: test_library test_program

test_library: test_library.c
	$(CC) $(CFLAGS) $@.c -shared -fPIC $(LDFLAGS) -o $@.so

test_program: test_program.c
	$(CC) $(CFLAGS) $@.c -ldl -o $@

all: insertion test

clean:
	rm *.o ./insertion
