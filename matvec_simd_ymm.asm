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
    ;case: m or n is <= 0 (C passes these as 32-bit ints)
    test ecx, ecx
    jle .done
    test edx, edx
    jle .done
    mov r10, [rsp+40]   ;Y pointer
    ;null pointer checks
    test r10, r10
    jz .done
    test r8, r8
    jz .done
    test r9, r9
    jz .done

    ;8 floats per YMM
    mov r11, rdx
    shr r11, 3    ;n / 8 to remove the remainder
    shl r11, 3    ;perfect batches of 8 cols

.row_loop:
    vxorps ymm4, ymm4, ymm4    ;init accumulator for this row
    xor rax, rax              ;processed cols ctr

.col_loop:
    ;all batches done
    cmp rax, r11
    jge .handle_rem

    ;vector product
    vmovdqu ymm0, [r8+rax*4]  ;8 from A (256ymm bits/ 32float bits)
    vmovdqu ymm1, [r9+rax*4]  ;8 from X (256ymm bits/ 32float bits)
    vmulps ymm2, ymm0, ymm1
    vaddps ymm4, ymm4, ymm2

    add rax, 8  ;8 cols processed (256 bits)
    jmp .col_loop

.handle_rem:
    ;combine halves first because horizontal adds stay within each 128 bit half
    vextractf128 xmm3, ymm4, 1
    vaddps xmm4, xmm4, xmm3
    ;now do another horizontal add
    vhaddps xmm4, xmm4, xmm4
    vhaddps xmm4, xmm4, xmm4    ;low lane now holds the sum of all 8 lanes

.rem_loop:
    ;remaining cols
    
    ;check if all rem cols processed
    cmp rax, rdx
    jge .assign

    ;summation A*X for remaining cols
    vmovss xmm0, [r8+rax*4]
    vmovss xmm1, [r9+rax*4]
    vmulss xmm2, xmm0, xmm1
    vaddss xmm4, xmm4, xmm2

    inc rax
    jmp .rem_loop

.assign:
    vmovss [r10], xmm4

    add r10, 4  ; move to next float in Y
    lea r8, [r8+rdx*4]    ;point to next row of A
    
    dec rcx               ;one less row remaining
    jnz .row_loop

.done:
    xor rax, rax
    ret
