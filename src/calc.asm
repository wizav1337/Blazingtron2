; calc.asm - Blazingtron 2
; NASM x64 assembly for high-performance price calculations (SSE2)
; Called from C code using Microsoft x64 calling convention
;
; double calc_vip_price(double service, int discount_percent);
; double calc_booster_cut(double vip_price, int cut_percent);

default rel
global calc_vip_price
global calc_booster_cut

section .text

; double calc_vip_price(double service /*xmm0*/, int discount /*rdx*/)
calc_vip_price:
    push    rbp
    mov     rbp, rsp

    cvtsi2sd xmm1, rdx          ; discount int -> double (xmm1)
    movsd   xmm2, [hundred]     ; 100.0
    subsd   xmm2, xmm1          ; 100 - discount
    mulsd   xmm0, xmm2          ; service * (100 - d)
    divsd   xmm0, [hundred]     ; / 100

    pop     rbp
    ret

; double calc_booster_cut(double vip /*xmm0*/, int cut /*rdx*/)
calc_booster_cut:
    push    rbp
    mov     rbp, rsp

    cvtsi2sd xmm1, rdx          ; cut percent -> double
    mulsd   xmm0, xmm1          ; vip * cut
    divsd   xmm0, [hundred]     ; / 100

    pop     rbp
    ret

section .rdata
hundred:    dq 100.0
