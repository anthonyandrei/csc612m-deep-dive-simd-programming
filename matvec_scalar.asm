; DeepDive SIMD Programming
; Group 5
; Members:
;   - Julian Johan Briones
;   - Ramon John Dela Cruz
;   - Anthony Andrei Tan

default rel
section .text

global matvec_scalar

matvec_scalar:
    ; Validate dimensions (m > 0 and n > 0)
    test rcx, rcx
    jle .done
    test rdx, rdx
    jle .done

    ; Retrieve 5th argument (pointer to Y) from caller shadow stack frame
    mov r10, [rsp + 40]
    test r10, r10
    jz .done

    xor r11, r11                        ; i = 0 (row counter)

.row_loop:
    xorps xmm0, xmm0                    ; sum = 0.0f
    xor rax, rax                        ; j = 0 (column counter)

.col_loop:
    movss xmm1, [r8]                    ; xmm1 = A[i * n + j]
    mulss xmm1, [r9 + rax*4]            ; xmm1 = A[i * n + j] * X[j]
    addss xmm0, xmm1                    ; sum += A[i * n + j] * X[j]

    add r8, 4                           ; advance A pointer to next float
    inc rax                             ; j++
    cmp rax, rdx
    jl .col_loop

    movss [r10 + r11*4], xmm0           ; Y[i] = sum
    inc r11                             ; i++
    cmp r11, rcx
    jl .row_loop

.done:
    ret
