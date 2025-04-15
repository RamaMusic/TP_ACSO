%define NULL 0
%define TRUE 1
%define FALSE 0

section .data

section .text

global string_proc_list_create_asm
global string_proc_node_create_asm
global string_proc_list_add_node_asm
global string_proc_list_concat_asm

extern malloc
extern free
extern strlen
extern strcat
extern memset

; Creates a new string processing list
; Returns: pointer to the new list or NULL if error
string_proc_list_create_asm:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 16
        mov     edi, 16
        call    malloc
        mov     QWORD [rbp-8], rax
        cmp     QWORD [rbp-8], 0
        jne     .malloc_success
        mov     eax, 0
        jmp     .return
.malloc_success:
        mov     rax, QWORD [rbp-8]
        mov     edx, 16
        mov     esi, 0
        mov     rdi, rax
        call    memset
        mov     rax, QWORD [rbp-8]
.return:
        leave
        ret

; Creates a new node for string processing
; Parameters: edi = type, rsi = string pointer
; Returns: pointer to the new node or NULL if error
string_proc_node_create_asm:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 32
        mov     eax, edi
        mov     QWORD [rbp-32], rsi
        mov     BYTE [rbp-20], al
        cmp     QWORD [rbp-32], 0
        jne     .valid_string
        mov     eax, 0
        jmp     .return_node
.valid_string:
        mov     edi, 32
        call    malloc
        mov     QWORD [rbp-8], rax
        cmp     QWORD [rbp-8], 0
        jne     .node_malloc_success
        mov     eax, 0
        jmp     .return_node
.node_malloc_success:
        mov     rax, QWORD [rbp-8]
        mov     rdx, QWORD [rbp-32]
        mov     QWORD [rax+24], rdx
        mov     rax, QWORD [rbp-8]
        movzx   edx, BYTE [rbp-20]
        mov     BYTE [rax+16], dl
        mov     rax, QWORD [rbp-8]
        mov     QWORD [rax], 0
        mov     rax, QWORD [rbp-8]
        mov     QWORD [rax+8], 0
        mov     rax, QWORD [rbp-8]
.return_node:
        leave
        ret

; Adds a node to a string processing list
; Parameters: rdi = list pointer, esi = type, rdx = string pointer
; Returns: void
string_proc_list_add_node_asm:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 48
        mov     QWORD [rbp-24], rdi
        mov     eax, esi
        mov     QWORD [rbp-40], rdx
        mov     BYTE [rbp-28], al
        cmp     QWORD [rbp-24], 0
        je      .invalid_params
        cmp     QWORD [rbp-40], 0
        je      .invalid_params
        movzx   eax, BYTE [rbp-28]
        mov     rdx, QWORD [rbp-40]
        mov     rsi, rdx
        mov     edi, eax
        call    string_proc_node_create_asm
        mov     QWORD [rbp-8], rax
        cmp     QWORD [rbp-8], 0
        je      .node_create_failed
        mov     rax, QWORD [rbp-24]
        mov     rax, QWORD [rax]
        test    rax, rax
        jne     .list_not_empty
        mov     rax, QWORD [rbp-24]
        mov     rdx, QWORD [rbp-8]
        mov     QWORD [rax], rdx
        mov     rax, QWORD [rbp-24]
        mov     rdx, QWORD [rbp-8]
        mov     QWORD [rax+8], rdx
        jmp     .add_node_return
.list_not_empty:
        mov     rax, QWORD [rbp-24]
        mov     rax, QWORD [rax+8]
        test    rax, rax
        jne     .add_to_existing_list
        mov     rax, QWORD [rbp-8]
        mov     rdi, rax
        call    free
        jmp     .add_node_return
.add_to_existing_list:
        mov     rax, QWORD [rbp-24]
        mov     rdx, QWORD [rax+8]
        mov     rax, QWORD [rbp-8]
        mov     QWORD [rax+8], rdx
        mov     rax, QWORD [rbp-24]
        mov     rax, QWORD [rax+8]
        mov     rdx, QWORD [rbp-8]
        mov     QWORD [rax], rdx
        mov     rax, QWORD [rbp-24]
        mov     rdx, QWORD [rbp-8]
        mov     QWORD [rax+8], rdx
        jmp     .add_node_return
.invalid_params:
        nop
        jmp     .add_node_return
.node_create_failed:
        nop
.add_node_return:
        leave
        ret

; Concatenates strings in a list based on type
; Parameters: rdi = list pointer, esi = type, rdx = base string
; Returns: pointer to concatenated string or NULL if error
string_proc_list_concat_asm:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 896
        mov     QWORD [rbp-872], rdi
        mov     eax, esi
        mov     QWORD [rbp-888], rdx
        mov     BYTE [rbp-876], al
        cmp     QWORD [rbp-872], 0
        je      .concat_invalid_params
        cmp     QWORD [rbp-888], 0
        jne     .concat_valid_params
.concat_invalid_params:
        mov     eax, 0
        jmp     .concat_return
.concat_valid_params:
        mov     rax, QWORD [rbp-888]
        mov     rdi, rax
        call    strlen
        mov     QWORD [rbp-48], rax
        cmp     QWORD [rbp-48], 0
        je      .invalid_length
        mov     rax, QWORD [rbp-48]
        test    rax, rax
        jns     .valid_length
.invalid_length:
        mov     eax, 0
        jmp     .concat_return
.valid_length:
        mov     rax, QWORD [rbp-48]
        mov     QWORD [rbp-8], rax
        mov     rax, QWORD [rbp-872]
        mov     rax, QWORD [rax]
        mov     QWORD [rbp-16], rax
        lea     rdx, [rbp-864]
        mov     eax, 0
        mov     ecx, 100
        mov     rdi, rdx
        rep stosq
        mov     QWORD [rbp-24], 0
        jmp     .first_loop_check
.first_loop_body:
        mov     QWORD [rbp-32], 0
        jmp     .inner_loop_check
.inner_loop_body:
        mov     rax, QWORD [rbp-32]
        mov     rax, QWORD [rbp-864+rax*8]
        cmp     QWORD [rbp-16], rax
        jne     .not_duplicate
        mov     eax, 0
        jmp     .concat_return
.not_duplicate:
        add     QWORD [rbp-32], 1
.inner_loop_check:
        mov     rax, QWORD [rbp-32]
        cmp     rax, QWORD [rbp-24]
        jb      .inner_loop_body
        cmp     QWORD [rbp-24], 99
        ja      .skip_tracking
        mov     rax, QWORD [rbp-24]
        lea     rdx, [rax+1]
        mov     QWORD [rbp-24], rdx
        mov     rdx, QWORD [rbp-16]
        mov     QWORD [rbp-864+rax*8], rdx
.skip_tracking:
        mov     rax, QWORD [rbp-16]
        movzx   eax, BYTE [rax+16]
        cmp     BYTE [rbp-876], al
        jne     .type_mismatch
        mov     rax, QWORD [rbp-16]
        mov     rax, QWORD [rax+24]
        test    rax, rax
        je      .type_mismatch
        mov     rax, QWORD [rbp-16]
        mov     rax, QWORD [rax+24]
        mov     rdi, rax
        call    strlen
        mov     QWORD [rbp-64], rax
        mov     rax, QWORD [rbp-8]
        not     rax
        cmp     QWORD [rbp-64], rax
        jb      .length_ok
        mov     eax, 0
        jmp     .concat_return
.length_ok:
        mov     rax, QWORD [rbp-64]
        add     QWORD [rbp-8], rax
.type_mismatch:
        mov     rax, QWORD [rbp-16]
        mov     rax, QWORD [rax]
        mov     QWORD [rbp-16], rax
.first_loop_check:
        cmp     QWORD [rbp-16], 0
        jne     .first_loop_body
        mov     rax, QWORD [rbp-8]
        add     rax, 1
        mov     rdi, rax
        call    malloc
        mov     QWORD [rbp-56], rax
        cmp     QWORD [rbp-56], 0
        jne     .result_malloc_success
        mov     eax, 0
        jmp     .concat_return
.result_malloc_success:
        mov     rax, QWORD [rbp-56]
        mov     BYTE [rax], 0
        mov     rdx, QWORD [rbp-888]
        mov     rax, QWORD [rbp-56]
        mov     rsi, rdx
        mov     rdi, rax
        call    strcat
        mov     rax, QWORD [rbp-872]
        mov     rax, QWORD [rax]
        mov     QWORD [rbp-16], rax
        mov     QWORD [rbp-24], 0
        lea     rax, [rbp-864]
        mov     edx, 800
        mov     esi, 0
        mov     rdi, rax
        call    memset
        jmp     .second_loop_check
.second_loop_body:
        mov     QWORD [rbp-40], 0
        jmp     .second_inner_check
.second_inner_body:
        mov     rax, QWORD [rbp-40]
        mov     rax, QWORD [rbp-864+rax*8]
        cmp     QWORD [rbp-16], rax
        jne     .not_duplicate_second
        mov     rax, QWORD [rbp-56]
        mov     rdi, rax
        call    free
        mov     eax, 0
        jmp     .concat_return
.not_duplicate_second:
        add     QWORD [rbp-40], 1
.second_inner_check:
        mov     rax, QWORD [rbp-40]
        cmp     rax, QWORD [rbp-24]
        jb      .second_inner_body
        cmp     QWORD [rbp-24], 99
        ja      .skip_tracking_second
        mov     rax, QWORD [rbp-24]
        lea     rdx, [rax+1]
        mov     QWORD [rbp-24], rdx
        mov     rdx, QWORD [rbp-16]
        mov     QWORD [rbp-864+rax*8], rdx
.skip_tracking_second:
        mov     rax, QWORD [rbp-16]
        movzx   eax, BYTE [rax+16]
        cmp     BYTE [rbp-876], al
        jne     .skip_concat
        mov     rax, QWORD [rbp-16]
        mov     rax, QWORD [rax+24]
        test    rax, rax
        je      .skip_concat
        mov     rax, QWORD [rbp-16]
        mov     rdx, QWORD [rax+24]
        mov     rax, QWORD [rbp-56]
        mov     rsi, rdx
        mov     rdi, rax
        call    strcat
.skip_concat:
        mov     rax, QWORD [rbp-16]
        mov     rax, QWORD [rax]
        mov     QWORD [rbp-16], rax
.second_loop_check:
        cmp     QWORD [rbp-16], 0
        jne     .second_loop_body
        mov     rax, QWORD [rbp-56]
.concat_return:
        leave
        ret