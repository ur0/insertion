global inject_library
global nullsub

section .data
section .text

inject_library:
; rdi -> Pointer to malloc()
; rsi -> Pointer to free()
; rdx -> Pointer to dlopen()
; rcx -> Size of the path to the .so to load

; Create a new stack frame
push rbp

; Save rbx because we're using it as scratch space
push rbx
; Save addresses of free & dlopen on the stack
push rsi
push rdx

; Move the pointer to malloc into r9
mov r9, rdi
; Move the size of the path as the first argument to malloc
mov rdi, rcx
; Align the stack pointer
mov rbx, rsp
sub rsp, 0x30
and spl, 0xF0
; Call malloc(so_path_size)
call r9
mov rsp, rbx
; Stop so that we can see what's happening from the injector process
int 0x3

; Move the pointer to dlopen into r9
pop r9
; Move the malloc'd space (now containing the path) to rdi for the first argument
mov rdi, rax
; Push rax because it'll be overwritten
push rax
; Second argument to dlopen (RTLD_LAZY)
mov rsi, 0x1
; Align the stack pointer
mov rbx, rsp
sub rsp, 0x30
and spl, 0xF0
; Call dlopen(path_to_library, RTLD_LAZY)
call r9
mov rsp, rbx
; Pass control to the injector
int 0x3

; Finally, begin free-ing the malloc'd area
pop rdi
; Get the address of free into r9
pop r9
; Call free(path_to_library)
call r9

; Restore rbx
pop rbx

; Destroy the stack frame
pop rbp

; We're done
int 0x3
retn

nullsub:
retn
