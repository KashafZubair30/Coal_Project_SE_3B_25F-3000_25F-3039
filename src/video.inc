; video.inc  -  Module 2: video mode, palette, PutPixel, FillRect, text
; Every drawing procedure loads ES from [fb_seg] itself and restores it, and
; preserves all registers it does not return.  Parameters are pushed in the order
; written in the header; the procedure removes them with RET n.
; InitVideo
;   purpose : switch to VGA mode 13h, read the ROM 8x8 font pointer, load our own palette (index 16 and up)
;   inputs  : none
;   outputs : font_seg and font_off set; video mode 13h active
;   changes : none (pusha/popa, ES saved)
InitVideo:
    pusha
    push es
    mov  ax, 0013h
    int  10h
    mov  ax, 1130h              ; get font pointer
    mov  bh, 03h                ; ROM 8x8 font (characters 0..127)
    push bp
    int  10h
    mov  [font_off], bp
    mov  [font_seg], es
    pop  bp
    push cs
    pop  es
    mov  dx, 3C8h               ; DAC write index
    mov  al, PAL_FIRST
    out  dx, al
    inc  dx                     ; 3C9h data port
    mov  si, palette
    mov  cx, PAL_COUNT*3
    cld
    rep  outsb
    pop  es
    popa
    ret
; PutPixel(x, y, colour)
;   purpose : write one pixel into the frame buffer, clipped to the screen
;   inputs  : [bp+8] = x, [bp+6] = y (signed words), [bp+4] = colour (low byte)
;   outputs : one pixel in the frame buffer (nothing when outside the screen)
;   changes : none (AX, BX, ES are saved); removes its 3 parameters (ret 6)
PutPixel:
    push bp
    mov  bp, sp
    push ax
    push bx
    push es
    mov  ax, [bp+8]
    cmp  ax, SCREEN_W
    jae  .done                  ; unsigned compare also rejects negatives
    mov  bx, [bp+6]
    cmp  bx, SCREEN_H
    jae  .done
    mov  es, [fb_seg]
    mov  ax, bx
    shl  bx, 8                  ; y*256
    shl  ax, 6                  ; y*64
    add  bx, ax                 ; y*320
    add  bx, [bp+8]
    mov  al, [bp+4]
    mov  [es:bx], al
.done:
    pop  es
    pop  bx
    pop  ax
    pop  bp
    ret  6
; FillRect(x, y, w, h, colour)
;   purpose : filled rectangle in the frame buffer, clipped to the screen (REP STOSB per row)
;   inputs  : [bp+12] = x, [bp+10] = y, [bp+8] = w, [bp+6] = h (signed words), [bp+4] = colour
;   outputs : rectangle drawn into the frame buffer
;   changes : none (pusha/popa, ES saved); removes its 5 parameters (ret 10)
FillRect:
    push bp
    mov  bp, sp
    pusha
    push es
    mov  es, [fb_seg]
    mov  si, [bp+12]            ; x0
    mov  bx, si
    add  bx, [bp+8]             ; x1 = x + w
    test si, si
    jns  .x0ok
    xor  si, si
.x0ok:
    cmp  bx, SCREEN_W
    jle  .x1ok
    mov  bx, SCREEN_W
.x1ok:
    sub  bx, si
    jle  .done
    mov  dx, bx                 ; DX = clipped width
    mov  ax, [bp+10]            ; y0
    mov  bx, ax
    add  bx, [bp+6]             ; y1
    test ax, ax
    jns  .y0ok
    xor  ax, ax
.y0ok:
    cmp  bx, SCREEN_H
    jle  .y1ok
    mov  bx, SCREEN_H
.y1ok:
    sub  bx, ax
    jle  .done
    mov  cx, bx                 ; CX = rows
    mov  di, ax
    shl  ax, 6
    shl  di, 8
    add  di, ax                 ; y*320
    add  di, si
    mov  al, [bp+4]
    cld
.row:
    push cx
    push di
    mov  cx, dx
    rep  stosb
    pop  di
    pop  cx
    add  di, SCREEN_W
    loop .row
.done:
    pop  es
    popa
    pop  bp
    ret  10
; DrawChar(x, y, char, colour, scale)
;   purpose : draw one 8x8 ROM-font glyph, each font pixel scaled to scale x scale
;   inputs  : [bp+12] = x, [bp+10] = y, [bp+8] = character code, [bp+6] = colour, [bp+4] = scale
;   outputs : glyph drawn into the frame buffer
;   changes : none (pusha/popa, ES saved); removes its 5 parameters (ret 10)
DrawChar:
    push bp
    mov  bp, sp
    pusha
    push es
    sub  sp, 4                  ; [bp-20] = cur_y   [bp-22] = cur_x   (see below)
    mov  si, [bp+8]
    and  si, 00FFh
    shl  si, 3                  ; 8 bytes per glyph
    add  si, [font_off]
    mov  es, [font_seg]
    mov  ax, [bp+10]
    mov  [bp-20], ax
    xor  bx, bx                 ; font row 0..7
.rowloop:
    mov  dl, [es:si+bx]
    mov  ax, [bp+12]
    mov  [bp-22], ax
    mov  cx, 8
.colloop:
    test dl, 80h
    jz   .next
    push word [bp-22]
    push word [bp-20]
    push word [bp+4]
    push word [bp+4]
    push word [bp+6]
    call FillRect
.next:
    shl  dl, 1
    mov  ax, [bp+4]
    add  [bp-22], ax
    loop .colloop
    mov  ax, [bp+4]
    add  [bp-20], ax
    inc  bx
    cmp  bx, 8
    jb   .rowloop
    lea  sp, [bp-18]
    pop  es
    popa
    pop  bp
    ret  10
; DrawText(x, y, ptr, colour, scale)
;   purpose : draw a zero-terminated string with DrawChar, 8 x scale pixels per character
;   inputs  : [bp+12] = x, [bp+10] = y, [bp+8] = address of the string, [bp+6] = colour, [bp+4] = scale
;   outputs : string drawn into the frame buffer
;   changes : none (AX and BX are saved); removes its 5 parameters (ret 10). It advances its own copy of x on the stack
DrawText:
    push bp
    mov  bp, sp
    push ax
    push bx
    mov  bx, [bp+8]
.next:
    mov  al, [bx]
    test al, al
    jz   .done
    xor  ah, ah
    push word [bp+12]
    push word [bp+10]
    push ax
    push word [bp+6]
    push word [bp+4]
    call DrawChar
    mov  ax, [bp+4]
    shl  ax, 3
    add  [bp+12], ax            ; advance x (the pushed parameter is ours to change)
    inc  bx
    jmp  .next
.done:
    pop  bx
    pop  ax
    pop  bp
    ret  10
; DrawTextC(y, ptr, colour, scale)
;   purpose : draw a zero-terminated string centred horizontally: x = (320 - width) / 2
;   inputs  : [bp+10] = y, [bp+8] = address of the string, [bp+6] = colour, [bp+4] = scale
;   outputs : string drawn into the frame buffer
;   changes : none (AX, BX, DX are saved); removes its 4 parameters (ret 8)
DrawTextC:
    push bp
    mov  bp, sp
    push ax
    push bx
    push dx
    xor  ax, ax
    mov  bx, [bp+8]
.len:
    cmp  byte [bx], 0
    je   .got
    inc  ax
    inc  bx
    jmp  .len
.got:
    shl  ax, 3                  ; chars * 8
    mul  word [bp+4]            ; * scale  (DX:AX)
    mov  bx, SCREEN_W
    sub  bx, ax
    shr  bx, 1                  ; x = (320 - width) / 2
    push bx
    push word [bp+10]
    push word [bp+8]
    push word [bp+6]
    push word [bp+4]
    call DrawText
    pop  dx
    pop  bx
    pop  ax
    pop  bp
    ret  8
; FormatNum(low, high, digits, dest)
;   purpose : convert a 32-bit number to zero-padded ASCII by repeated division by 10 (remainders are pushed, then popped in reverse order)
;   inputs  : [bp+10] = low word, [bp+8] = high word, [bp+6] = number of digits, [bp+4] = address of the buffer
;   outputs : buffer holds the digits followed by a 0 byte
;   changes : none (pusha/popa); removes its 4 parameters (ret 8)
FormatNum:
    push bp
    mov  bp, sp
    pusha
    mov  ax, [bp+10]
    mov  dx, [bp+8]
    mov  si, [bp+6]
    mov  bx, 10
.divloop:
    mov  di, ax                 ; DI = low word
    mov  ax, dx                 ; high word
    xor  dx, dx
    div  bx                     ; AX = high/10, DX = high mod 10
    xchg ax, di                 ; AX = low word, DI = quotient high
    div  bx                     ; DX:AX / 10  -> AX = quotient low, DX = digit
    push dx
    mov  dx, di                 ; DX:AX = 32-bit quotient
    dec  si
    jnz  .divloop
    mov  di, [bp+4]
    mov  cx, [bp+6]
.poploop:
    pop  ax
    add  al, '0'
    mov  [di], al
    inc  di
    loop .poploop
    mov  byte [di], 0
    popa
    pop  bp
    ret  8
