#include <stdio.h>
#include "ptrace.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <wait.h>
#include "util.h"

void inject_library(void *malloc_addr, void *dlopen_addr, void* free_addr, int path_size);
void nullsub(void);
const size_t injector_len = 0x46;

int main(int argc, char* argv[]) {
	if (argc <= 2) {
		puts("Usage: ./insertion <PID> <library_path>");
		exit(-1);
	}

	char* pid_str = argv[1];
	char* library_path = argv[2];
	char* real_path = realpath(library_path, NULL);

	pid_t target_pid = (pid_t)atoi(pid_str);
	printf("Targeting process with ID: %ld\n", target_pid);
	
	size_t path_len = strlen(real_path) + 1;

	unsigned long int libc_base_addr = get_library_addr(target_pid, "libc-");
	unsigned long int malloc_addr = libc_base_addr + 0x000000000007e600;
	unsigned long int free_addr = libc_base_addr + 0x000000000007ea90;

	unsigned long int libdl_base_addr = get_library_addr(target_pid, "libdl-");
	unsigned long int dlopen_addr = libdl_base_addr + 0x0000000000000fe0;

	printf("Target process' libc is at 0x%x\n", libc_base_addr);
	printf("Target process' malloc is at 0x%x\n", malloc_addr);

	struct user_regs_struct oldregs, regs;
	memset(&oldregs, 0, sizeof(struct user_regs_struct));
	memset(&regs, 0, sizeof(struct user_regs_struct));

	ptrace_attach(target_pid);
	ptrace_getregs(target_pid, &oldregs);
	memcpy(&regs, &oldregs, sizeof(struct user_regs_struct));

	unsigned long int injection_addr = get_injection_addr(target_pid);
	printf("Injecting at 0x%x\n", injection_addr);

	regs.rip = injection_addr + 2;
	regs.rdi = malloc_addr;
	regs.rsi = free_addr;
	regs.rdx = dlopen_addr;
	regs.rcx = path_len;
	ptrace_setregs(target_pid, &regs);

	void *existing_code = malloc(injector_len);
	ptrace_read(target_pid, injection_addr, existing_code, injector_len);

	void *injector_code = malloc(injector_len);
	memset(injector_code, 0, injector_len);
	memcpy(injector_code, (void *)inject_library, injector_len);

	ptrace_write(target_pid, injection_addr, injector_code, injector_len);
	puts("Wrote our injector, continuing execution until malloc breakpoint");
	ptrace_cont(target_pid);

	memset(&regs, 0, sizeof(struct user_regs_struct));
	ptrace_getregs(target_pid, &regs);
	unsigned long int library_location_buf = regs.rax;
	if (!library_location_buf) {
		puts("ERROR: Failed to allocate memory for library path.");
		exit(-1);
	}
	free(injector_code);

	puts("malloc() in target process succeeded, now copying over target library's location");
	ptrace_write(target_pid, library_location_buf, real_path, path_len);
	puts("Library path written, continuing");
	ptrace_cont(target_pid);

	memset(&regs, 0, sizeof(struct user_regs_struct));
	ptrace_getregs(target_pid, &regs);
	if (!regs.rax) {
		puts("ERROR: Failed to dlopen(). Check your library.");
		exit(-1);
	}
	puts("dlopen() in target process succeeded. Freeing allocated buffers.");
	ptrace_cont(target_pid);

	puts("free'd the buffer allocated for the library name. Restoring original process state.");
	ptrace_write(target_pid, injection_addr, existing_code, injector_len);
	free(existing_code);
	ptrace_setregs(target_pid, &oldregs);
	puts("Process state restored. We're done.");
	ptrace_detach(target_pid);

	return 0;
}
