; DrawClouds
;   purpose : bonus parallax: clouds scroll at half the camera speed 
;   inputs  : none (reads cam_x and cloud_table)
;   outputs : clouds drawn into the frame buffer
;   changes : none (pusha/popa)

DrawClouds:
    pusha
    mov  si, cloud_table         
    mov  bx, [cam_x]
    shr  bx, 1
.loop:
    mov  ax, [si]
    test ax, ax
    jz   .done
    sub  ax, bx                  ; screen x of the cloud
    cmp  ax, -48
    jl   .next
    cmp  ax, SCREEN_W
    jge  .next
    mov  di, ax                  ; DI = screen x of the cloud
    mov  dx, [si+2]              ; DX = y
    add  ax, 8
    push ax
    push dx
    push word 24
    push word 8
    push word COL_WHITE
    call FillRect                ; top bump
    push di
    mov  ax, dx
    add  ax, 8
    push ax
    push word 40
    push word 8
    push word COL_WHITE
    call FillRect                ; wide base
.next:
    add  si, 4
    jmp  .loop
.done:
    popa
    ret

; DrawCastle
;   purpose : draw a simple castle behind Mario at the end of the level
;   inputs  : none (reads cam_x)
;   outputs : castle drawn into the frame buffer when it is on screen
;   changes : none (pusha/popa)

DrawCastle:
    pusha
    mov  ax, CASTLE_X
    sub  ax, [cam_x]
    cmp  ax, SCREEN_W
    jge  .out
    mov  si, ax
    push si
    push word 112
    push word 32
    push word 48
    push word C_BRICK
    call FillRect               ; wall
    mov  cx, 4
    mov  dx, si
.battlement:
    push dx
    push word 104
    push word 6
    push word 8
    push word C_BRICK
    call FillRect               ; battlements
    add  dx, 9
    loop .battlement
    add  si, 10
    push si
    push word 136
    push word 12
    push word 24
    push word COL_BLACK
    call FillRect               ; door
.out:
    popa
    ret

; DrawEntities
;   purpose : draw enemies, shells, mushrooms and popping coins
;   inputs  : none (reads the entity table, cam_x and frame_ctr)
;   outputs : entities drawn into the frame buffer
;   changes : none (pusha/popa)

DrawEntities:
    pusha
    mov  si, entities
    mov  cx, MAX_ENT
.loop:
    cmp  byte [si+E_TYPE], 0
    je   .next
    mov  dx, [si+E_X]
    sub  dx, [cam_x]            ; DX = screen x
    cmp  dx, -16
    jle  .next
    cmp  dx, SCREEN_W
    jge  .next
    xor  ax, ax                 ; AX = flags
    mov  bl, [si+E_TYPE]
    cmp  bl, ET_GOOMBA
    jne  .check_koopa
    mov  di, spr_goomba
    cmp  byte [si+E_STATE], ES_FLAT
    jne  .goomba_walk
    mov  di, spr_goomba_flat
    jmp  .draw
.goomba_walk:
    mov  ax, [frame_ctr]
    shr  ax, 3
    and  ax, 1                  ; mirror every 8 frames = walking animation
    jmp  .draw
.check_koopa:
    cmp  bl, ET_KOOPA
    jne  .check_mush
    mov  di, spr_koopa
    cmp  byte [si+E_STATE], ES_WALK
    je   .koopa_face
    mov  di, spr_shell
    jmp  .draw
.koopa_face:
    cmp  byte [si+E_DIR], 0
    jg   .draw
    mov  ax, 1                  ; facing left = mirrored
    jmp  .draw
.check_mush:
    cmp  bl, ET_MUSH
    jne  .coin
    mov  di, spr_mush
    jmp  .draw
.coin:
    mov  di, tiles_art+(T_COIN-1)*256
.draw:
    push dx
    mov  bx, [si+E_Y]
    sub  bx, 16
    push bx
    push di
    push ax
    call DrawSprite
.next:
    add  si, E_SIZE
    loop .loop
    popa
    ret

; DrawMario
;   purpose : draw Mario with the right pose facing, form (Small or Super) and blinking while invulnerable
;   inputs  : none (reads Mario variables, cam_x, frame_ctr, state)
;   outputs : Mario drawn into the frame buffer 
;   changes : none (pusha/popa)

DrawMario:
    pusha
    cmp  byte [m_invuln], 0
    je   .show
    test byte [frame_ctr], 1
    jnz  .out                   ; draw on alternate frames only
.show:
    cmp  word [state], ST_WIN
    jne  .vis
    cmp  word [m_x], CASTLE_X+8
    jge  .out                   ; walked into the castle door
.vis:
    mov  dx, [m_x]
    sub  dx, [cam_x]            ; screen x
    mov  bx, [m_y]
    sub  bx, [m_h]              ; screen y of the top
    xor  ax, ax                 ; pose 0 = stand
    cmp  byte [m_ground], 0
    jne  .grounded
    mov  ax, 2                  ; jump
    jmp  .pose
.grounded:
    cmp  word [m_vx], 0
    je   .pose
    mov  ax, [frame_ctr]
    shr  ax, 2
    and  ax, 1                  ; alternate stand / run frames
.pose:
    xor  cx, cx                 ; flags
    cmp  byte [m_face], 0
    je   .nomirror
    mov  cx, 1
.nomirror:
    cmp  byte [m_form], 0
    jne  .super
    shl  ax, 8                  ; 256 bytes per small sprite
    add  ax, spr_small_stand
    jmp  .go
.super:
    shl  ax, 9                  ; 512 bytes per super sprite
    add  ax, spr_super_stand
    or   cx, 2
.go:
    push dx
    push bx
    push ax
    push cx
    call DrawSprite
.out:
    popa
    ret

; RenderWorld
;   purpose : clear, clouds, map, castle, entities, Mario, HUD, black bottom strip, then copy to the screen
;   inputs  : none
;   outputs : one finished frame on the screen
;   changes : none (every procedure it calls preserves its registers)

RenderWorld:
    call ClearFrame
    call DrawClouds
    call DrawMap
    call DrawCastle
    call DrawEntities
    call DrawMario
    call DrawHUD
    push word 0
    push word VIEW_H
    push word SCREEN_W
    push word SCREEN_H-VIEW_H
    push word COL_BLACK
    call FillRect               ; bottom 8 pixel rows stay black
    call FlipScreen
    ret