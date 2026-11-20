; InitMemory
;   purpose : shrink the .COM block to 64 KB  and allocate the 64,000-byte frame buffer 
;   inputs  : none (ES must still hold the PSP segment)
;   outputs : CF = 0 and fb_seg = segment of the buffer, or CF = 1 on failure
;   changes : flags only (AX and BX are saved)

InitMemory:
    push ax
    push bx
    mov  bx, 1000h              ; keep 64 KB (code + data + stack)
    mov  ah, 4Ah
    int  21h                    ; ES = PSP segment at program start
    jc   .fail
    mov  bx, FB_PARAS
    mov  ah, 48h
    int  21h
    jc   .fail
    mov  [fb_seg], ax
    clc
    jmp  .out
.fail:
    stc
.out:
    pop  bx
    pop  ax
    ret

; ClearFrame
;   purpose : fill the whole frame buffer with the sky colour using one REP STOSW 
;   inputs  : none (reads fb_seg)
;   outputs : frame buffer = sky colour
;   changes : none (AX, CX, DI, ES are saved)

ClearFrame:
    push ax
    push cx
    push di
    push es
    mov  es, [fb_seg]
    xor  di, di
    mov  ax, C_SKY*257          ; two pixels per word
    mov  cx, 32000
    cld
    rep  stosw
    pop  es
    pop  di
    pop  cx
    pop  ax
    ret

; FlipScreen
;   purpose : copy the finished frame buffer to video memory with one REP MOVSW
;   inputs  : none (reads fb_seg)
;   outputs : screen shows the frame
;   changes : none (AX, CX, SI, DI, DS, ES are saved)

FlipScreen:
    push ax
    push cx
    push si
    push di
    push ds
    push es
    mov  ax, VIDEO_SEG
    mov  es, ax
    mov  ds, [fb_seg]
    xor  si, si
    xor  di, di
    mov  cx, 32000
    cld
    rep  movsw
    pop  es
    pop  ds
    pop  di
    pop  si
    pop  cx
    pop  ax
    ret
