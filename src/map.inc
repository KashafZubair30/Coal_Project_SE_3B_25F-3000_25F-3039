; map.inc  -  Module 3: level map, GetTile / SetTile / SolidAt, DrawTile, DrawMap
; GetTile(px, py)
;   purpose : convert a pixel position to the tile code of the WORKING map (row*60 + column with shifts)
;   inputs  : [bp+6] = pixel x, [bp+4] = pixel y, pushed in that order
;   outputs : AX = tile code; above or below the map = sky (0), beyond the level sides = solid ground
;   changes : AX only (BX and CX are saved); removes its 2 parameters (ret 4)
GetTile:
    push bp
    mov  bp, sp
    push bx
    push cx
    mov  ax, [bp+4]             ; py
    cmp  ax, VIEW_H
    jae  .sky
    mov  bx, [bp+6]             ; px
    cmp  bx, WORLD_W
    jae  .wall
    shr  ax, 4                  ; row
    mov  cx, ax
    shl  ax, 6
    shl  cx, 2
    sub  ax, cx                 ; row*64 - row*4 = row*60
    shr  bx, 4                  ; column
    add  bx, ax
    mov  al, [map_work+bx]      ; base + index
    xor  ah, ah
    jmp  .out
.sky:
    xor  ax, ax
    jmp  .out
.wall:
    mov  ax, T_GROUND
.out:
    pop  cx
    pop  bx
    pop  bp
    ret  4
; SolidAt(px, py)
;   purpose : test whether the tile at a pixel position is solid (tile codes 1..10)
;   inputs  : [bp+6] = pixel x, [bp+4] = pixel y
;   outputs : AX = 1 when solid, else 0
;   changes : AX only; removes its 2 parameters (ret 4)
SolidAt:
    push bp
    mov  bp, sp
    push word [bp+6]
    push word [bp+4]
    call GetTile
    dec  ax
    cmp  ax, 10
    jb   .solid
    xor  ax, ax
    jmp  .out
.solid:
    mov  ax, 1
.out:
    pop  bp
    ret  4
; SetTile(tx, ty, code)
;   purpose : change one tile of the working map
;   inputs  : [bp+8] = tile column tx, [bp+6] = tile row ty, [bp+4] = new tile code
;   outputs : map_work changed
;   changes : none (AX and BX are saved); removes its 3 parameters (ret 6)
SetTile:
    push bp
    mov  bp, sp
    push ax
    push bx
    mov  ax, [bp+6]             ; ty
    mov  bx, ax
    shl  ax, 6
    shl  bx, 2
    sub  ax, bx                 ; ty*60
    add  ax, [bp+8]             ; + tx
    mov  bx, ax
    mov  al, [bp+4]
    mov  [map_work+bx], al
    pop  bx
    pop  ax
    pop  bp
    ret  6
; ResetMap
;   purpose : copy the original map over the working copy with one REP MOVSB
;   inputs  : none
;   outputs : map_work = map_orig
;   changes : none (CX, SI, DI, ES are saved and restored)
ResetMap:
    push cx
    push si
    push di
    push es
    push ds
    pop  es
    cld
    mov  si, map_orig
    mov  di, map_work
    mov  cx, MAP_SIZE
    rep  movsb
    pop  es
    pop  di
    pop  si
    pop  cx
    ret
; DrawTile(x, y, code)
;   purpose : draw one 16x16 tile, clipped on the left and right; opaque tiles (1..10) are copied row by row with REP MOVSB, the others go through DrawSprite
;   inputs  : [bp+8] = x (signed), [bp+6] = y, [bp+4] = tile code
;   outputs : tile drawn into the frame buffer
;   changes : none (pusha/popa, ES saved); removes its 3 parameters (ret 6)
DrawTile:
    push bp
    mov  bp, sp
    pusha
    push es
    mov  es, [fb_seg]
    mov  bx, [bp+4]
    cmp  bx, T_COIN
    jae  .sprite
    dec  bx
    mov  si, bx
    shl  si, 8
    add  si, tiles_art
    mov  ax, [bp+8]             ; x
    xor  dx, dx
    mov  cx, 16
    test ax, ax
    jns  .xpos
    mov  dx, ax
    neg  dx                     ; pixels skipped on the left
    mov  cx, 16
    sub  cx, dx
    add  si, dx
    xor  ax, ax
    jmp  .xdone
.xpos:
    cmp  ax, SCREEN_W-16
    jbe  .xdone
    mov  cx, SCREEN_W
    sub  cx, ax                 ; partly off the right edge
.xdone:
    mov  di, [bp+6]
    mov  bx, di
    shl  di, 8
    shl  bx, 6
    add  di, bx
    add  di, ax
    mov  bx, 16
    cld
.row:
    push cx
    push si
    push di
    rep  movsb
    pop  di
    pop  si
    pop  cx
    add  si, 16
    add  di, SCREEN_W
    dec  bx
    jnz  .row
    jmp  .done
.sprite:
    dec  bx
    shl  bx, 8
    add  bx, tiles_art
    push word [bp+8]
    push word [bp+6]
    push bx
    push word 0
    call DrawSprite
.done:
    pop  es
    popa
    pop  bp
    ret  6
; DrawMap
;   purpose : draw the visible part of the map: 20 columns + 1 partial one; first column = cam_x / 16, pixel offset = -(cam_x mod 16)
;   inputs  : none (reads cam_x and map_work)
;   outputs : all non-empty visible tiles drawn into the frame buffer
;   changes : none (pusha/popa)
DrawMap:
    pusha
    mov  cx, [cam_x]
    shr  cx, 4                  ; CX = first map column
    mov  dx, [cam_x]
    and  dx, 15
    neg  dx                     ; DX = x offset of the first column
    xor  si, si                 ; screen column 0..20
.col:
    mov  ax, cx
    add  ax, si
    cmp  ax, MAP_W
    jae  .done
    mov  di, ax
    add  di, MAP_W              ; start at map row 1 (row 0 is the HUD)
    mov  bx, 1
.row:
    cmp  byte [map_work+di], 0
    je   .skip
    mov  ax, si
    shl  ax, 4
    add  ax, dx
    push ax                     ; x
    mov  ax, bx
    shl  ax, 4
    push ax                     ; y
    mov  al, [map_work+di]
    xor  ah, ah
    push ax                     ; code
    call DrawTile
.skip:
    add  di, MAP_W
    inc  bx
    cmp  bx, MAP_H
    jb   .row
    inc  si
    cmp  si, 21
    jb   .col
.done:
    popa
    ret

%include "level.inc"
