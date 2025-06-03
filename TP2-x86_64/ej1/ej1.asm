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
        mov     edi, 16; Arg del malloc
        call    malloc
        mov     QWORD [rbp-8], rax ; ret del malloc
        cmp     QWORD [rbp-8], 0
        jne     .malloc_success
        mov     eax, 0
        jmp     .return
.malloc_success:
        mov     rax, QWORD [rbp-8]
        mov     edx, 16 ; Args c
        mov     esi, 0; args b
        mov     rdi, rax; args a
        call    memset; (a, b, c)
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

        mov     eax, edi                 ; guarda type en eax
        mov     QWORD [rbp-32], rsi      ; guarda hash
        mov     BYTE [rbp-20], al        ; guarda type (1 byte)

        cmp     QWORD [rbp-32], 0        ; si hash == NULL → return NULL
        jne     .valid_string
        mov     eax, 0
        jmp     .return_node

.valid_string:
        mov     edi, 32                  ; Arg malloc
        call    malloc
        mov     QWORD [rbp-8], rax       ; ret malloc

        cmp     QWORD [rbp-8], 0         ; si NULL → return NULL
        jne     .node_malloc_success
        mov     eax, 0
        jmp     .return_node

.node_malloc_success:
        mov     rax, QWORD [rbp-8]
        mov     rdx, QWORD [rbp-32]
        mov     QWORD [rax+24], rdx      ; node->hash = hash

        mov     rax, QWORD [rbp-8]
        movzx   edx, BYTE [rbp-20]
        mov     BYTE [rax+16], dl        ; node->type = type

        mov     rax, QWORD [rbp-8]
        mov     QWORD [rax], 0           ; node->next = NULL

        mov     rax, QWORD [rbp-8]
        mov     QWORD [rax+8], 0         ; node->prev = NULL

        mov     rax, QWORD [rbp-8]       ; return node
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

        mov     QWORD [rbp-24], rdi      ; list
        mov     eax, esi
        mov     QWORD [rbp-40], rdx      ; hash
        mov     BYTE [rbp-28], al        ; type (1 byte)

        cmp     QWORD [rbp-24], 0        ; si list == NULL
        je      .invalid_params
        cmp     QWORD [rbp-40], 0        ; si hash == NULL
        je      .invalid_params

        movzx   eax, BYTE [rbp-28]
        mov     rdx, QWORD [rbp-40]
        mov     rsi, rdx                 ; arg2 = hash
        mov     edi, eax                 ; arg1 = type
        call    string_proc_node_create_asm

        mov     QWORD [rbp-8], rax       ; node
        cmp     QWORD [rbp-8], 0         ; si node == NULL
        je      .node_create_failed

        mov     rax, QWORD [rbp-24]      ; list
        mov     rax, QWORD [rax]         ; list->first
        test    rax, rax                 ; first == 0
        jne     .list_not_empty          ; si no está vacía

        ; lista vacía: first = last = node
        mov     rax, QWORD [rbp-24]
        mov     rdx, QWORD [rbp-8]
        mov     QWORD [rax], rdx         ; list->first = node
        mov     rax, QWORD [rbp-24]
        mov     rdx, QWORD [rbp-8]
        mov     QWORD [rax+8], rdx       ; list->last = node
        jmp     .add_node_return

.list_not_empty:
        mov     rax, QWORD [rbp-24]
        mov     rax, QWORD [rax+8]       ; list->last
        test    rax, rax
        jne     .add_to_existing_list

        ; si last == NULL → liberar el nodo
        mov     rax, QWORD [rbp-8]
        mov     rdi, rax
        call    free
        jmp     .add_node_return

.add_to_existing_list:
        mov     rax, QWORD [rbp-24]
        mov     rdx, QWORD [rax+8]       ; list->last
        mov     rax, QWORD [rbp-8]
        mov     QWORD [rax+8], rdx       ; node->previous = list->last

        mov     rax, QWORD [rbp-24]
        mov     rax, QWORD [rax+8]       ; list->last
        mov     rdx, QWORD [rbp-8]
        mov     QWORD [rax], rdx         ; list->last->next = node

        mov     rax, QWORD [rbp-24]
        mov     rdx, QWORD [rbp-8]
        mov     QWORD [rax+8], rdx       ; list->last = node

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
        sub     rsp, 896                 ; reserva stack frame grande

        mov     QWORD [rbp-872], rdi     ; list
        mov     eax, esi
        mov     QWORD [rbp-888], rdx     ; hash
        mov     BYTE [rbp-876], al       ; type

        cmp     QWORD [rbp-872], 0       ; list == null
        je      .concat_invalid_params
        cmp     QWORD [rbp-888], 0       ; hash == null
        jne     .concat_valid_params

.concat_invalid_params:
        mov     eax, 0                   ; return null
        jmp     .concat_return

.concat_valid_params:
        mov     rax, QWORD [rbp-888]     ; hash
        mov     rdi, rax
        call    strlen                   ; strlen(hash)
        mov     QWORD [rbp-48], rax      ; hash_len

        cmp     QWORD [rbp-48], 0        ; hash vacío
        je      .invalid_length
        mov     rax, QWORD [rbp-48]
        test    rax, rax
        jns     .valid_length            ; jns = no negativo

.invalid_length:
        mov     eax, 0                   ; return null
        jmp     .concat_return
.valid_length:
        mov     rax, QWORD [rbp-48]         ; hash_len
        mov     QWORD [rbp-8], rax          ; total_len = hash_len

        mov     rax, QWORD [rbp-872]
        mov     rax, QWORD [rax]            ; list->first
        mov     QWORD [rbp-16], rax         ; current

        lea     rdx, [rbp-864]              ; visited[], dirección, no valor
        mov     eax, 0
        mov     ecx, 100
        mov     rdi, rdx
        rep stosq                           ; limpiar visited con ceros

        mov     QWORD [rbp-24], 0           ; visit_count = 0
        jmp     .first_loop_check

.first_loop_body:
        mov     QWORD [rbp-32], 0           ; i = 0 (índice de búsqueda en visited)
        jmp     .inner_loop_check

.inner_loop_body:
        mov     rax, QWORD [rbp-32]
        mov     rax, QWORD [rbp-864+rax*8]  ; visited[i]
        cmp     QWORD [rbp-16], rax         ; current == visited[i] ?
        jne     .not_duplicate

        mov     eax, 0                      ; ciclo detectado → return null
        jmp     .concat_return

.not_duplicate:
        add     QWORD [rbp-32], 1           ; i++
.inner_loop_check:
        mov     rax, QWORD [rbp-32]          ; i
        cmp     rax, QWORD [rbp-24]          ; i < visit_count ?
        jb      .inner_loop_body

        cmp     QWORD [rbp-24], 99           ; visit_count < 100 ?
        ja      .skip_tracking

        mov     rax, QWORD [rbp-24]
        lea     rdx, [rax+1]                 ; visit_count++
        mov     QWORD [rbp-24], rdx

        mov     rdx, QWORD [rbp-16]          ; current
        mov     QWORD [rbp-864+rax*8], rdx   ; visited[visit_count-1] = current

.skip_tracking:
        mov     rax, QWORD [rbp-16]          ; current
        movzx   eax, BYTE [rax+16]           ; current->type (uint8_t → extendido)
        cmp     BYTE [rbp-876], al           ; type == current->type ?
        jne     .type_mismatch

        mov     rax, QWORD [rbp-16]
        mov     rax, QWORD [rax+24]          ; current->hash
        test    rax, rax                     ; null ?
        je      .type_mismatch

        mov     rax, QWORD [rbp-16]
        mov     rax, QWORD [rax+24]          ; current->hash
        mov     rdi, rax
        call    strlen                       ; strlen(current->hash)
        mov     QWORD [rbp-64], rax          ; node_len

        mov     rax, QWORD [rbp-8]           ; total_len
        not     rax
        cmp     QWORD [rbp-64], rax          ; overflow? (node_len < ~total_len)
        jb      .length_ok

        mov     eax, 0
        jmp     .concat_return

.length_ok:
        mov     rax, QWORD [rbp-64]
        add     QWORD [rbp-8], rax           ; total_len += node_len

.type_mismatch:
        mov     rax, QWORD [rbp-16]
        mov     rax, QWORD [rax]             ; current = current->next
        mov     QWORD [rbp-16], rax

.first_loop_check:
        cmp     QWORD [rbp-16], 0            ; current == NULL?
        jne     .first_loop_body

        mov     rax, QWORD [rbp-8]           ; total_len
        add     rax, 1                       ; +1 para '\0'
        mov     rdi, rax
        call    malloc                       ; reservar result
        mov     QWORD [rbp-56], rax

        cmp     QWORD [rbp-56], 0            ; malloc falló?
        jne     .result_malloc_success
        mov     eax, 0
        jmp     .concat_return

.result_malloc_success:
        mov     rax, QWORD [rbp-56]          ; result
        mov     BYTE [rax], 0                ; result[0] = '\0'

        mov     rdx, QWORD [rbp-888]         ; hash original
        mov     rax, QWORD [rbp-56]          ; result
        mov     rsi, rdx
        mov     rdi, rax
        call    strcat                       ; strcat(result, hash)

        mov     rax, QWORD [rbp-872]
        mov     rax, QWORD [rax]             ; current = list->first
        mov     QWORD [rbp-16], rax
        mov     QWORD [rbp-24], 0            ; visit_count = 0

        lea     rax, [rbp-864]               ; visited[]
        mov     edx, 800
        mov     esi, 0
        mov     rdi, rax
        call    memset                       ; limpiar visited[]

        jmp     .second_loop_check

.second_loop_body:
        mov     QWORD [rbp-40], 0            ; i = 0
        jmp     .second_inner_check

.second_inner_body:
        mov     rax, QWORD [rbp-40]
        mov     rax, QWORD [rbp-864+rax*8]   ; visited[i]
        cmp     QWORD [rbp-16], rax          ; current == visited[i]?
        jne     .not_duplicate_second

        mov     rax, QWORD [rbp-56]          ; result
        mov     rdi, rax
        call    free                         ; liberar result
        mov     eax, 0
        jmp     .concat_return

.not_duplicate_second:
        add     QWORD [rbp-40], 1            ; i++

.second_inner_check:
        mov     rax, QWORD [rbp-40]           ; i
        cmp     rax, QWORD [rbp-24]           ; i < visit_count ?
        jb      .second_inner_body

        cmp     QWORD [rbp-24], 99            ; visit_count < 100 ?
        ja      .skip_tracking_second

        mov     rax, QWORD [rbp-24]
        lea     rdx, [rax+1]                  ; visit_count++
        mov     QWORD [rbp-24], rdx
        mov     rdx, QWORD [rbp-16]           ; current
        mov     QWORD [rbp-864+rax*8], rdx    ; visited[visit_count-1] = current

.skip_tracking_second:
        mov     rax, QWORD [rbp-16]
        movzx   eax, BYTE [rax+16]            ; current->type
        cmp     BYTE [rbp-876], al            ; type == current->type ?
        jne     .skip_concat

        mov     rax, QWORD [rbp-16]
        mov     rax, QWORD [rax+24]           ; current->hash
        test    rax, rax                      ; hash == NULL?
        je      .skip_concat

        mov     rax, QWORD [rbp-16]
        mov     rdx, QWORD [rax+24]           ; current->hash
        mov     rax, QWORD [rbp-56]           ; result
        mov     rsi, rdx
        mov     rdi, rax
        call    strcat                        ; strcat(result, current->hash)

.skip_concat:
        mov     rax, QWORD [rbp-16]
        mov     rax, QWORD [rax]              ; current = current->next
        mov     QWORD [rbp-16], rax

.second_loop_check:
        cmp     QWORD [rbp-16], 0             ; while (current != NULL)
        jne     .second_loop_body

        mov     rax, QWORD [rbp-56]           ; return result

.concat_return:
        leave
        ret
