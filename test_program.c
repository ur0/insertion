#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char* argv) {
	pid_t my_pid = getpid();
	printf("PID: %ld\n", my_pid);
	getchar();
	return 0;
}
