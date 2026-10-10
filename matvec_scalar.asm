; DeepDive SIMD Programming
; Group 5
; Members:
;   - Julian Johan Briones
;   - Ramon John Dela Cruz
;   - Anthony Andrei Tan

default rel
section .text

global matvec_scalar

; ==============================================================================
; Function: matvec_scalar
; Computes the matrix-vector product (Y = A * X) using non-SIMD scalar x86-64 instructions.
;
; Parameters (Microsoft x64 Calling Convention):
;   RCX      = m (number of rows in matrix A)
;   RDX      = n (number of columns in matrix A / elements in vector X)
;   R8       = ptr A (pointer to row-major single-precision float matrix A)
;   R9       = ptr X (pointer to single-precision float input vector X)
;   [RSP+40] = ptr Y (pointer to single-precision float output vector Y)
;
; Register Usage:
;   RCX      : Row count m (matrix rows)
;   RDX      : Column count n (matrix columns)
;   R8       : Running pointer advancing sequentially through elements of matrix A
;   R9       : Base pointer to input vector X
;   R10      : Base pointer to output vector Y
;   R11      : Row loop counter i (0 <= i < m)
;   RAX      : Column loop counter j (0 <= j < n)
;   XMM0     : Scalar accumulator (sum) for the current row dot product
;   XMM1     : Temporary scalar float holding A[i * n + j] * X[j]
; ==============================================================================
matvec_scalar:
    ; Validate dimensions (m > 0 and n > 0)
    test ecx, ecx
    jle .done
    test edx, edx
    jle .done

    ; Retrieve 5th argument (pointer to Y) from caller shadow stack frame
    mov r10, [rsp + 40]

    ; Validate pointers (ensure non-null)
    test r10, r10
    jz .done
    test r8, r8
    jz .done
    test r9, r9
    jz .done

    xor r11, r11                        ; i = 0 (initialize row counter)

.row_loop:
    xorps xmm0, xmm0                    ; sum = 0.0f (clear accumulator for current row)
    xor rax, rax                        ; j = 0 (initialize column counter)

.col_loop:
    ; Compute scalar product: A[i * n + j] * X[j]
    movss xmm1, [r8]                    ; Load single float from matrix: xmm1 = A[i * n + j]
    mulss xmm1, [r9 + rax*4]            ; Multiply by vector element: xmm1 = A[i * n + j] * X[j]
    addss xmm0, xmm1                    ; Accumulate dot product: sum += A[i * n + j] * X[j]

    add r8, 4                           ; Advance matrix A pointer to the next float
    inc rax                             ; j++ (advance column counter)
    cmp rax, rdx
    jl .col_loop                        ; Continue column loop if j < n

    ; Store computed dot product into output vector Y[i]
    movss [r10 + r11*4], xmm0           ; Y[i] = sum
    inc r11                             ; i++ (advance row counter)
    cmp r11, rcx
    jl .row_loop                        ; Continue row loop if i < m

.done:
    xor rax, rax
    ret
