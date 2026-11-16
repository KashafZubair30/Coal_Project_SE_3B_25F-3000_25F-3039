; sprite.inc  -  Module 4: DrawSprite and the art conversion tables
; BuildArtTables
;   purpose : build the letter -> colour table (chartab) from chartab_init and convert ALL art (letters) to palette indices in place with XLAT. Must run exactly once at start-up (a second run would turn every pixel transparent)
;   inputs  : none (reads chartab_init, art_start  art_end)
;   outputs : chartab filled; every art byte replaced by its palette index
;   changes : none (pusha/popa, ES saved)

BuildArtTables:
    pusha
    push es
    push ds
    pop  es
    cld
    mov  di, chartab
    mov  cx, 256
    xor  al, al
    rep  stosb
    mov  si, chartab_init
.pairs:
    lodsb
    test al, al
    jz   .convert
    xor  bh, bh
    mov  bl, al
    lodsb
    mov  [chartab+bx], al
    jmp  .pairs
.convert:
    mov  si, art_start
    mov  di, si
    mov  cx, art_end-art_start
    mov  bx, chartab
.conv:
    lodsb
    xlat                        ; AL = chartab[AL]
    stosb
    loop .conv
    pop  es
    popa
    ret
; DrawSprite(x, y, address, flags)
;   purpose : draw a 16x16 or 16x32 sprite, colour index 0 = transparent, clipped on all four sides. flags: bit 0 = mirror left/right, bit 1 = tall (32 rows), bit 2 = half-transparent (checkerboard)
;   inputs  : [bp+10] = x, [bp+8] = y (signed words), [bp+6] = address of the sprite data, [bp+4] = flags
;   outputs : sprite drawn into the frame buffer
;   changes : none (pusha/popa, ES saved); removes its 4 parameters (ret 8)
DrawSprite:
    push bp
    mov  bp, sp
    pusha
    push es
    sub  sp, 10                 ; locals: [bp-20] c0  [bp-22] c1  [bp-24] rows  [bp-26] parity  [bp-28] y
    mov  es, [fb_seg]
    mov  ax, [bp+10]            ; x
    xor  cx, cx
    test ax, ax
    jns  .c0ok
    mov  cx, ax
    neg  cx
.c0ok:
    mov  [bp-20], cx            ; first visible column
    mov  dx, SCREEN_W
    sub  dx, ax
    cmp  dx, 16
    jle  .c1ok
    mov  dx, 16
.c1ok:
    mov  [bp-22], dx            ; end column (exclusive)
    cmp  cx, dx
    jge  .exit
    mov  cx, 16
    test word [bp+4], 2
    jz   .rowsok
    mov  cx, 32
.rowsok:
    mov  [bp-24], cx
    mov  si, [bp+6]             ; source row pointer
    mov  ax, [bp+8]
    mov  [bp-28], ax            ; y of the current row
.rowloop:
    mov  ax, [bp-28]
    cmp  ax, SCREEN_H
    jae  .nextrow               ; row above or below the screen
    mov  bx, ax
    shl  ax, 8
    shl  bx, 6
    add  ax, bx
    add  ax, [bp+10]
    mov  dx, ax                 ; DX = y*320 + x
    mov  ax, [bp+10]
    add  ax, [bp-28]
    mov  [bp-26], ax
    mov  bx, [bp-20]            ; BX = column
.pix:
    cmp  bx, [bp-22]
    jge  .nextrow
    mov  di, bx
    test word [bp+4], 1
    jz   .src
    mov  di, 15
    sub  di, bx                 ; mirrored source column
.src:
    add  di, si
    mov  al, [di]               ; source pixel = row pointer + column
    test al, al
    jz   .skip
    test word [bp+4], 4
    jz   .draw
    mov  cx, [bp-26]
    add  cx, bx
    test cl, 1
    jnz  .skip
.draw:
    mov  di, dx
    add  di, bx
    mov  [es:di], al
.skip:
    inc  bx
    jmp  .pix
.nextrow:
    add  si, 16
    inc  word [bp-28]
    dec  word [bp-24]
    jnz  .rowloop
.exit:
    lea  sp, [bp-18]
    pop  es
    popa
    pop  bp
    ret  8
