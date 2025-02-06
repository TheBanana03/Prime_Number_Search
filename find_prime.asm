section .text
global find_prime
default rel

find_prime:
    sub rsp, 16
    mov rbx, rsp
    add rsp, 16

    mov dword [rbx], 0
    vpbroadcastd ymm2, [rbx]

    sub rsp, 16
    mov rcx, rsp
    add rsp, 16

    mov dword [rcx], edi
    vpbroadcastd ymm0, [rcx]
    vmovdqu ymm1, [rdx]
    vmovdqa ymm5, ymm0

    vcvtdq2ps ymm0, ymm0
    
    vdivps ymm3, ymm0, ymm1
    ; vmovdqu [rsi], ymm3

    vcvtps2dq ymm3, ymm3
    vcvtps2dq ymm1, ymm1
    vpmulld ymm4, ymm3, ymm1
    ; vmovdqu [rsi], ymm4
    vpsubd ymm4, ymm5, ymm4
    ; vmovdqu [rsi], ymm4
    vpcmpeqd ymm4, ymm4, ymm2

    ; vmovdqu [rsi], ymm4
    vptest ymm4, ymm4
    setz al
    mov [rsi], al

    ret