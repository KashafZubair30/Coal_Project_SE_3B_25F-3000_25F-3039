; InstallKeyboard
;   purpose : save the old INT 09h vector and install our own handler 
;   inputs  : none
;   outputs : old_int9 = old vector; INT 09h now points to Int09Handler
;   changes : none (pusha/popa, ES saved)

InstallKeyboard:
    pusha
    push es
    mov  ax, 3509h
    int  21h
    mov  [old_int9], bx
    mov  [old_int9+2], es
    mov  dx, Int09Handler
    mov  ax, 2509h
    int  21h
    pop  es
    popa
    ret

; Int09Handler 
;   purpose : read port 60h; keep a 128-byte key-state table (1 = held) 
;   inputs  : scan code from port 60h
;   outputs : keys[] and keys_hit[] updated
;   changes : none (AX, BX, DS are saved)

Int09Handler:
    push ax
    push bx
    push ds
    mov  ax, cs
    mov  ds, ax
    in   al, 60h
    cmp  al, 0E0h
    jne  .notE0
    mov  byte [kb_ext], 1
    jmp  .eoi
.notE0:
    cmp  al, 0E1h
    je   .eoi
    mov  bl, al
    and  bl, 7Fh                ; scan code without the release bit
    cmp  byte [kb_ext], 0
    je   .normal
    mov  byte [kb_ext], 0
    cmp  bl, 2Ah                
    je   .eoi
    cmp  bl, 36h
    je   .eoi
.normal:
    xor  bh, bh
    test al, 80h
    jnz  .release
    cmp  byte [keys+bx], 0
    jne  .held
    mov  byte [keys_hit+bx], 1  ; fresh press 
.held:
    mov  byte [keys+bx], 1
    jmp  .eoi
.release:
    mov  byte [keys+bx], 0
.eoi:
    mov  al, 20h
    out  20h, al
    pop  ds
    pop  bx
    pop  ax
    iret

; TestHit
;   purpose : tell whether a key was freshly pressed since the last call, and clear that flag
;   inputs  : [bp+4] = scan code 
;   outputs : AX = 1 if the key was pressed since the last call, else 0
;   changes : AX only (BX is saved); removes its parameter (ret 2)

TestHit:
    push bp
    mov  bp, sp
    push bx
    mov  bx, [bp+4]
    xor  ax, ax
    xchg al, [keys_hit+bx]
    pop  bx
    pop  bp
    ret  2

; ClearHits
;   purpose : forget all pending key presses 
;   inputs  : none
;   outputs : keys_hit = 0 for all 128 keys
;   changes : none (AX, CX, DI, ES are saved)

ClearHits:
    push ax
    push cx
    push di
    push es
    push ds
    pop  es
    mov  di, keys_hit
    mov  cx, 128
    xor  al, al
    cld
    rep  stosb
    pop  es
    pop  di
    pop  cx
    pop  ax
    ret

; InstallTimer
;   purpose : save the old INT 08h vector, install Int08Handler
;   inputs  : none
;   outputs : old_int8 = old vector; PIT divisor = PIT_DIVISOR
;   changes : none (pusha/popa, ES saved)

InstallTimer:
    pusha
    push es
    mov  ax, 3508h
    int  21h
    mov  [old_int8], bx
    mov  [old_int8+2], es
    mov  dx, Int08Handler
    mov  ax, 2508h
    int  21h
    cli
    mov  al, 36h                ; channel 0, lo/hi byte, square wave
    out  43h, al
    mov  ax, PIT_DIVISOR
    out  40h, al
    mov  al, ah
    out  40h, al
    sti
    pop  es
    popa
    ret

; Int08Handler 
;   purpose : count ticks; 
;   inputs  : none (hardware interrupt)
;   outputs : ticks + 1; tick_acc updated
;   changes : none (AX and DS are saved)

Int08Handler:
    push ax
    push ds
    mov  ax, cs
    mov  ds, ax
    inc  word [ticks]
    add  word [tick_acc], 39773 ; 
    pop  ds
    pop  ax
    jc   .chain
    push ax
    mov  al, 20h
    out  20h, al
    pop  ax
    iret
.chain:
    jmp  far [cs:old_int8]      ; original handler ends with IRET for us

; WaitForTick
;   purpose : fixed game speed (30 frames per second)
;   inputs  : none (reads ticks and last_tick)
;   outputs : last_tick = ticks, frame_ctr + 1
;   changes : none (AX is saved)

WaitForTick:
    push ax
.wait:
    mov  ax, [ticks]
    cmp  ax, [last_tick]
    jne  .go
    hlt
    jmp  .wait
.go:
    mov  [last_tick], ax
    inc  word [frame_ctr]
    pop  ax
    ret

; Cleanup
;   purpose : restore the timer rate , both interrupt vectors and text mode
;   inputs  : none (reads old_int8 and old_int9)
;   outputs : program ends with INT 21h AX=4C00h, it does not return
;   changes : not applicable (does not return)

Cleanup:
    cli
    mov  al, 36h
    out  43h, al
    xor  al, al
    out  40h, al               
    out  40h, al
    push ds
    lds  dx, [old_int8]
    mov  ax, 2508h
    int  21h
    pop  ds
    push ds
    lds  dx, [old_int9]
    mov  ax, 2509h
    int  21h
    pop  ds
    sti
    mov  ax, 0003h
    int  10h
    mov  ax, 4C00h
    int  21h
