bits 64 ; set bit mode

; ensure labels available to linker
global swap
global start
global stop

%macro swap_macro 0
  mov qword [rdi+0x00], rsp
  mov qword [rdi+0x08], r15
  mov qword [rdi+0x10], r14
  mov qword [rdi+0x18], r13
  mov qword [rdi+0x20], r12
  mov qword [rdi+0x28], rbx
  mov qword [rdi+0x30], rbp

  mov qword rsp, [rsi+0x00]
  mov qword r15, [rsi+0x08]
  mov qword r14, [rsi+0x10]
  mov qword r13, [rsi+0x18]
  mov qword r12, [rsi+0x20]
  mov qword rbx, [rsi+0x28]
  mov qword rbp, [rsi+0x30]
%endmacro

section .text ; code section
swap:
  swap_macro
  mov qword rax, 0 ; return 0 aka false
  ret

start:
  swap_macro
  ret

stop:
  swap_macro
  mov qword rax, 1 ; return 1 aka true
  ret