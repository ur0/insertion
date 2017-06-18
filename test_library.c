#include <stdio.h>

static void init() __attribute__((constructor));

void init() {
	puts("loaded!");
}
