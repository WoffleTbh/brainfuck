; Brainfuck compiler
; Compiles Brainfuck to x86-64 assembly

BITS 64

global _start

section .bss
    src: resb 660000
    out: resb 660000
    out_p: resq 1
    src_file_fd: resq 1
    out_file_fd: resq 1

section .rodata
    usage: db "Usage: brainfuck <source file>", 10
    len_usage: equ $ - usage
    file_not_found: db "File not found", 10
    len_file_not_found: equ $-file_not_found
    out_file: db "out.asm", 0
    ; Specific to compilation
    ins_add: db "add ", 0
    ins_sub: db "sub ", 0
    ins_mov: db "mov ", 0
    dump_byte: db "mov rax, 1", 10, "mov rdi, 1", 10, "mov rsi, [rbx]", 10, "mov rdx, 1", 10, "syscall", 10, 0
    len_dump_byte: equ $ - dump_byte

    reg_rax: db "rax", 0
    reg_rbx: db "rbx", 0
    reg_rcx: db "rcx", 0
    reg_rdx: db "rdx", 0
    reg_rdi: db "rdi", 0
    reg_rsi: db "rsi", 0

    const_1: db "1", 0

    comma: db ",", 0

section .text

emit_str:
    ; writes a string to `out'
    ; string is passed in rax
    mov rcx, 0
.loop:
    mov rbx, [rax+rcx] ; get char
    cmp rbx, 0 ; end of string?
    je .end_loop
    mov rdx, [out_p] ; get pointer to current position in out
    mov [out+rdx], rbx ; write char to out
    add rcx, 1 ; next char
    add rdx, 1 ; next position in out
    mov [out_p], rdx ; set new position in out
    jmp .loop
.end_loop:
    ret

compile:
    mov QWORD [out_p], 0
    mov rbx, 0
    mov rcx, 0
.loop:

    ; [src+rbx] = current char
    cmp BYTE [src+rbx], '+'
    jne .if_end
.plus:
    mov rax, [ins_add]
    call emit_str
    mov rax, [reg_rbx]
    call emit_str
    mov rax, [comma]
    call emit_str
    mov rax, [const_1]
    call emit_str
    jmp .if_end

.if_end:
    add rbx, 1
    cmp BYTE [src+rbx], 0
    jne .loop
    ret

error_usage:
    mov rax, 1
    mov rdi, 1
    mov rsi, usage
    mov rdx, len_usage
    syscall
    mov rax, 60
    mov rdi, 1
    syscall

_start:
    ; Collect argv
    pop rax
    cmp rax, 2
    jl error_usage
    ; copy the first argument to the file variable
    pop rax ; discard the program name
    pop rax ; copy the first argument to the file variable
    ; open the file
    mov rdi, rax
    mov rsi, 0
    mov rdx, 0777
    mov rax, 2
    syscall
    mov QWORD [src_file_fd], rax
    ; check for errors
    cmp rax, -1
    jne .file_open_success
    ; error opening file
    mov rax, 1
    mov rdi, 1
    mov rsi, file_not_found
    mov rdx, len_file_not_found
    syscall
    mov rax, 60
    mov rdi, 1
    syscall
.file_open_success:
    ; Load the source file into memory
    mov rax, 0
    mov rdi, [src_file_fd]
    mov rsi, src
    mov rdx, 660000
    syscall
    ; Close the file
    mov rax, 3
    mov rdi, rax
    syscall
    ; Now we have the source code in memory, we can start compiling
    call compile
    ; Write the output to a file
    ; We open a new file for writing
    mov rax, 2
    mov rdi, out_file
    mov rsi, 85
    mov rdx, 0777
    syscall
    ; write the output to the file
    mov rdi, rax
    mov rsi, out
    mov rdx, 660000
    mov rax, 1
    syscall
    ; close the file
    mov rax, 3
    mov rdi, rax
    syscall
    ; exit
    mov rax, 60
    mov rdi, 0
    syscall

