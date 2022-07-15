/* Brainfuck compiler
 * Compiles brainfuck to x86 assembly
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    char* code;
    size_t size;
    size_t addr;
} bfProgram;

bool emit_str(bfProgram* program, char* str) {
    size_t len = strlen(str);
    program->code = realloc(program->code, program->size + len + 1);
    if (program->code == NULL) {
        return false;
    }
    strcpy(program->code + program->size, str);
    program->size += len;
    return true;
}

char* compile(char* code) {
    char* start = code;
    bfProgram program = {
        .code = malloc(1),
        .size = 0,
        .addr = 0
    };
    emit_str(&program, "BITS 64\n");
    emit_str(&program, "section .data\n");
    emit_str(&program, "    newline: db 10\n");
    emit_str(&program, "section .bss\n");
    emit_str(&program, "    memory: resb 30000\n");
    emit_str(&program, "    print_buffer: resb 1024\n");
    emit_str(&program, "    print_buf_size: resq 1\n");
    emit_str(&program, "section .text\n");
    emit_str(&program, "print_byte:\n");
    emit_str(&program, ".add_to_buffer:\n");
    emit_str(&program, "    mov al, BYTE [rbx]\n");
    emit_str(&program, "    mov rcx, [print_buf_size]\n");
    emit_str(&program, "    inc QWORD [print_buf_size]\n");
    emit_str(&program, "    mov BYTE [print_buffer+rcx], al\n");
    emit_str(&program, "    cmp QWORD [print_buf_size], 1024\n");
    emit_str(&program, "    je .force_print\n");
    emit_str(&program, "    cmp al, 10\n");
    emit_str(&program, "    jne .print_byte_end\n");
    emit_str(&program, ".force_print:\n");
    emit_str(&program, "    mov rax, 1\n");
    emit_str(&program, "    mov rdi, 1\n");
    emit_str(&program, "    mov rsi, print_buffer\n");
    emit_str(&program, "    mov rdx, print_buf_size\n");
    emit_str(&program, "    syscall\n");
    emit_str(&program, ".print_byte_end:\n");
    emit_str(&program, "    ret\n");
    emit_str(&program, "global _start\n");
    emit_str(&program, "_start:\n");
    emit_str(&program, "    mov QWORD [print_buf_size], 0\n");
    emit_str(&program, "    mov rbx, memory\n");
    size_t addr_stack[256] = {0};
    size_t addr_stack_p = 0;
    while (*code) {
        switch (*code) {
            case '+': {
                size_t amnt = 1;
                while (*(code + 1) == '+') {
                    amnt += 1;
                    code++;
                }
                char* s_amnt = malloc(20);
                sprintf(s_amnt, "%zu", amnt);
                emit_str(&program, "    add BYTE [rbx], ");
                emit_str(&program, s_amnt);
                emit_str(&program, "\n");
                free(s_amnt);
                break;
            }
            case '-': {
                size_t amnt = 1;
                while (*(code + 1) == '-') {
                    amnt += 1;
                    code++;
                }
                char* s_amnt = malloc(20);
                sprintf(s_amnt, "%zu", amnt);
                emit_str(&program, "    sub BYTE [rbx], ");
                emit_str(&program, s_amnt);
                emit_str(&program, "\n");
                free(s_amnt);
                break;
            }
            case '>': {
                size_t amnt = 1;
                while (*(code + 1) == '>') {
                    amnt += 1;
                    code++;
                }
                char* s_amnt = malloc(20);
                sprintf(s_amnt, "%zu", amnt);
                emit_str(&program, "    add rbx, ");
                emit_str(&program, s_amnt);
                emit_str(&program, "\n");
                free(s_amnt);
                break;
            }
            case '<': {
                size_t amnt = 1;
                while (*(code + 1) == '<') {
                    amnt += 1;
                    code++;
                }
                char* s_amnt = malloc(20);
                sprintf(s_amnt, "%zu", amnt);
                emit_str(&program, "    sub rbx, ");
                emit_str(&program, s_amnt);
                emit_str(&program, "\n");
                free(s_amnt);
                break;
            }
            case '.':
                emit_str(&program, "    call print_byte\n");
                break;
            case ',':
                emit_str(&program, "    mov rax, 0\n");
                emit_str(&program, "    mov rdi, 0\n");
                emit_str(&program, "    mov rsi, rbx\n");
                emit_str(&program, "    mov rdx, 1\n");
                emit_str(&program, "    syscall\n");
                break;
            case '[':
                emit_str(&program, "    cmp BYTE [rbx], 0\n");
                char* addr = malloc(sizeof(char) * 20);
                sprintf(addr, "%zu", program.addr);
                emit_str(&program, "    je addr_");
                emit_str(&program, addr);
                emit_str(&program, "_end\n");
                addr_stack[addr_stack_p++] = program.addr++;
                emit_str(&program, "addr_");
                emit_str(&program, addr);
                emit_str(&program, ":\n");
                free(addr);
                break;
            case ']':
                if (addr_stack_p == 0) {
                    fprintf(stderr, "Error: ] without [ at index: %zu\n", code - start);
                    exit(1);
                }
                emit_str(&program, "    cmp BYTE [rbx], 0\n");
                char* linked_addr = malloc(sizeof(char) * 20);
                sprintf(linked_addr, "%zu", addr_stack[--addr_stack_p]);
                emit_str(&program, "    jne addr_");
                emit_str(&program, linked_addr);
                emit_str(&program, "\n");
                emit_str(&program, "addr_");
                emit_str(&program, linked_addr);
                emit_str(&program, "_end:\n");
                free(linked_addr);
                break;
        }
        code++;
    }
    emit_str(&program, "    mov rax, 60\n");
    emit_str(&program, "    mov rdi, 0\n");
    emit_str(&program, "    syscall\n");
    char* final = malloc(sizeof(char) * (program.size + 1));
    strcpy(final, program.code);
    free(program.code);
    return final;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }
    // look for "-k" in argv
    bool keep = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0) {
            keep = true;
        }
    }
    FILE* file = fopen(argv[1], "r");
    if (file == NULL) {
        printf("Could not open file %s\n", argv[1]);
        return 1;
    }
    char* code = malloc(sizeof(char) * 1024);
    size_t code_size = 0;
    while (fgets(code + code_size, 1024 - code_size, file) != NULL) {
        code_size += strlen(code + code_size);
    }
    fclose(file);
    char* output = compile(code);
    FILE* out = fopen("out.asm", "w");
    fprintf(out, "%s", output);
    fclose(out);
    free(output);
    free(code);
    system("nasm -f elf64 out.asm -o out.o");
    system("ld -o out out.o");
    if (!keep) {
        system("rm out.asm out.o");
    }
    return 0;
}
