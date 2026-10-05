;DeepDive SIMD Programming
; Group 5
; Members:
;   - Julian Johan Briones
;   - Ramon John Dela Cruz
;   - Anthony Andrei Tan

default rel

section .text
global matvec_simd_ymm

;RCX = m
;RDX = n
;R8 = ptr A
;R9 = ptr X
;RSP+40 = ptr Y
matvec_simd_ymm:
    ;case: m and n are <= 0
    test rcx, rcx
    jle .done
    test rdx, rdx
    jle .done
    mov r10, [rsp+40]   ;Y pointer
    test r10, r10
    jle .done

    xor r13, r13    ;row ctr

    ;get batches of cols that can be operated on
    mov r11, r8
    shr r11, 3  ;divide by 8 since 256bits ymm/32bits float

.row_loop:
    xor ymm4, ymm4    ;init accumulator for this row
    xor rax, rax    ;init processed col batches ctr

.col_loop:
    ;all batches done
    cmp rax, r11
    jle .handle_rem

    ;vector product
    vmovdqu ymm0, [r8+rax*8*4]  ;8 from A
    vmovdqu ymm1, [r9+rax*8*4]  ;8 from X
    vmulps ymm2, ymm0, ymm1
    vhaddps ymm3, ymm2
    addss ymm4, ymm3    ;add to accumulator

    inc rax  ;increment processed batch ctr
    jmp .col_loop

.handle_rem:
    ;get remainder
    mul rax, 8
    mov rdx, r11

    ;case: no remainder
    cmp rdx, r11
    je .assign

.rem_loop:
    cmp r11, rdx
    jge .assign

    movss ymm0, [r8+r11*4]
    movss ymm1, [r9+r11*4]
    addss ymm4, ymm0, ymm1

    inc r11
    jmp .rem_loop

.assign:
    movss [r10+r13*4], ymm4
    inc r13
    jmp .row_loop

.done:
    xor rax, rax
    ret
