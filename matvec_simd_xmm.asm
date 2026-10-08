;Review of Basic SIMD
; Group 5
; Members:
;   - Julian Johan Briones
;   - Ramon John Dela Cruz
;   - Anthony Andrei Tan

default rel

section .text
global matvec_simd_xmm

; RCX = m (rows)
; RDX = n (columns)
; R8  = ptr A
; R9  = ptr X
; RSP+40 = ptr Y
matvec_simd_xmm:

    ;Validate dimensions and pointers
    test ecx, ecx
    jle .done

    test edx, edx
    jle .done

    mov r10, [rsp+40];Y pointer (Output)
    
    test r10, r10
    jz .done

    test r8, r8
    jz .done

    test r9, r9
    jz .done

    ;Round n down to the nearest multiple of 4
    mov r11, rdx
    shr r11, 2
    shl r11, 2

.process_row:
    vxorps xmm4, xmm4, xmm4
    xor rax, rax

.vector_loop:
    cmp rax, r11
    jge .horizontal_sum

    ; Multiply four matrix and vector elements in parallel
    vmovdqu xmm0, [r8+rax*4]
    vmovdqu xmm1, [r9+rax*4]
    vmulps xmm2, xmm0, xmm1
    vaddps xmm4, xmm4, xmm2 ; Accumulate four partial sums

    add rax, 4              ; Advance by four columns
    jmp .vector_loop

.horizontal_sum:
    ; Combine the four SIMD lanes into one sum
    vhaddps xmm4, xmm4, xmm4
    vhaddps xmm4, xmm4, xmm4

.scalar_remainder_loop:
    cmp rax, rdx
    jge .store_result

    ; Process leftover columns one float at a time
    vmovss xmm0, [r8+rax*4]
    vmovss xmm1, [r9+rax*4]
    vmulss xmm2, xmm0, xmm1
    vaddss xmm4, xmm4, xmm2

    inc rax; Advance by one column
    jmp .scalar_remainder_loop

.store_result:
    vmovss [r10], xmm4; Store dot product in Y

    add r10, 4; Advance to next output element
    lea r8, [r8+rdx*4]; Advance to next matrix row

    dec rcx; Decrement remaining row count
    jnz .process_row

.done:
    xor rax, rax
    ret
