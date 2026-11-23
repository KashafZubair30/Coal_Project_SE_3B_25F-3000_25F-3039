; ResetHero
;   purpose : put Mario back at the level start as Small Mario
;   inputs  : none
;   outputs : Mario variables (position, speed, form, hp, flags) reset
;   changes : none

ResetHero:
    mov  word [m_x], 32
    mov  byte [m_xf], 0
    mov  word [m_y], GROUND_Y
    mov  word [m_prev_y], GROUND_Y
    mov  byte [m_yf], 0
    mov  word [m_vx], 0
    mov  word [m_vy], 0
    mov  byte [m_form], 0
    mov  byte [m_hp], 1
    mov  word [m_h], 16
    mov  byte [m_ground], 1
    mov  byte [m_invuln], 0
    mov  byte [m_chain], 0
    mov  byte [m_face], 0
    mov  byte [m_latch], 1      ; a held Space must be released before the first jump
    mov  byte [m_stomp_flag], 0
    ret

; MarioSolidV
;   purpose : test whether the pixel column px is solid at Mario's feet-1, middle or top
;   inputs  : [bp+4] = pixel x (world), pushed on the stack
;   outputs : AX = 1 if any of the three points is solid, else 0
;   changes : AX only (BX and CX are saved and restored); removes its parameter (ret 2)

MarioSolidV:
    push bp
    mov  bp, sp
    push bx
    push cx
    mov  cx, [m_y]
    dec  cx
    push word [bp+4]
    push cx
    call SolidAt
    test ax, ax
    jnz  .out
    mov  cx, [m_y]
    mov  bx, [m_h]
    shr  bx, 1
    sub  cx, bx
    push word [bp+4]
    push cx
    call SolidAt
    test ax, ax
    jnz  .out
    mov  cx, [m_y]
    sub  cx, [m_h]
    push word [bp+4]
    push cx
    call SolidAt
.out:
    pop  cx
    pop  bx
    pop  bp
    ret  2

; UpdateMario
;   purpose : acceleration, jump, gravity, then move X and Y one axis at a time with tile collision
;   inputs  : none (reads keys, Mario variables, cam_x)
;   outputs : Mario position, speed and flags updated; calls BlockHit when his head hits a block
;   changes : none (pusha/popa)

UpdateMario:
    pusha
    mov  ax, [m_y]
    mov  [m_prev_y], ax         ; previous feet position for the stomp test
    xor  dx, dx
    cmp  byte [keys+SC_RIGHT], 0
    je   .no_right
    inc  dx
.no_right:
    cmp  byte [keys+SC_LEFT], 0
    je   .no_left
    dec  dx
.no_left:
    mov  bx, MAX_WALK
    cmp  byte [keys+SC_LSHIFT], 0
    je   .got_max
    mov  bx, MAX_RUN
.got_max:
    mov  ax, [m_vx]
    test dx, dx
    jz   .friction
    jg   .go_right
    mov  byte [m_face], 1       ;  left
    mov  cx, ACCEL
    test ax, ax
    jle  .left_accel
    mov  cx, BRAKE              ; moving right: brake harder
.left_accel:
    sub  ax, cx
    neg  bx
    cmp  ax, bx
    jge  .set_vx
    mov  ax, bx
    jmp  .set_vx
.go_right:
    mov  byte [m_face], 0
    mov  cx, ACCEL
    test ax, ax
    jge  .right_accel
    mov  cx, BRAKE
.right_accel:
    add  ax, cx
    cmp  ax, bx
    jle  .set_vx
    mov  ax, bx
    jmp  .set_vx
.friction:
    test ax, ax
    jz   .set_vx
    jg   .friction_pos
    add  ax, FRICTION
    jle  .set_vx
    xor  ax, ax
    jmp  .set_vx
.friction_pos:
    sub  ax, FRICTION
    jge  .set_vx
    xor  ax, ax
.set_vx:
    mov  [m_vx], ax

    ;  jump (ground only, needs a fresh Space press) / variable height

    cmp  byte [keys+SC_SPACE], 0
    je   .space_released
    cmp  byte [m_latch], 0
    jne  .gravity
    mov  byte [m_latch], 1
    cmp  byte [m_ground], 0
    je   .gravity
    mov  word [m_vy], JUMP_V
    mov  byte [m_ground], 0
    jmp  .gravity
.space_released:
    mov  byte [m_latch], 0
    mov  ax, [m_vy]
    cmp  ax, JUMP_CUT
    jge  .gravity
    mov  word [m_vy], JUMP_CUT  ; released while rising fast: cut the jump short
.gravity:
    mov  ax, [m_vy]
    add  ax, GRAVITY
    cmp  ax, MAX_FALL
    jle  .vy_ok
    mov  ax, MAX_FALL
.vy_ok:
    mov  [m_vy], ax

    ; move X 

    mov  ax, [m_vx]
    mov  dx, ax
    add  [m_xf], dl
    mov  al, dh
    cbw
    adc  [m_x], ax
    mov  ax, [m_x]             
    cmp  ax, [cam_x]
    jge  .right_limit
    mov  ax, [cam_x]
    mov  [m_x], ax
    jmp  .x_stop
.right_limit:
    cmp  ax, MARIO_MAX_X
    jle  .tiles_x
    mov  word [m_x], MARIO_MAX_X
    jmp  .x_stop
.tiles_x:
    cmp  word [m_vx], 0
    je   .x_done
    jg   .wall_right
    mov  ax, [m_x]
    add  ax, 2                  ; left edge of the hitbox
    push ax
    call MarioSolidV
    test ax, ax
    jz   .x_done
    mov  ax, [m_x]
    add  ax, 2
    shr  ax, 4
    inc  ax
    shl  ax, 4
    sub  ax, 2                  ; stand against the right side of that tile
    mov  [m_x], ax
    jmp  .x_stop
.wall_right:
    mov  ax, [m_x]
    add  ax, 13                 ; right edge of the hitbox
    push ax
    call MarioSolidV
    test ax, ax
    jz   .x_done
    mov  ax, [m_x]
    add  ax, 13
    shr  ax, 4
    shl  ax, 4
    sub  ax, 14                 ; stand against the left side of that tile
    mov  [m_x], ax
.x_stop:
    mov  byte [m_xf], 0
    mov  word [m_vx], 0
.x_done:
    ;  move Y
    mov  ax, [m_vy]
    mov  dx, ax
    add  [m_yf], dl
    mov  al, dh
    cbw
    adc  [m_y], ax
    cmp  word [m_vy], 0
    jl   .rising
    mov  byte [m_ground], 0     ; falling: look under both feet
    mov  ax, [m_x]
    add  ax, 2
    push ax
    push word [m_y]
    call SolidAt
    test ax, ax
    jnz  .land
    mov  ax, [m_x]
    add  ax, 13
    push ax
    push word [m_y]
    call SolidAt
    test ax, ax
    jz   .y_done
.land:
    mov  ax, [m_y]
    shr  ax, 4
    shl  ax, 4                  ; top of the tile under the feet
    mov  [m_y], ax
    mov  byte [m_yf], 0
    mov  word [m_vy], 0
    mov  byte [m_ground], 1
    mov  byte [m_chain], 0      ; landing resets the stomp chain
    jmp  .y_done
.rising:
    mov  ax, [m_y]
    sub  ax, [m_h]
    mov  dx, ax                 ; DX = top pixel row of Mario
    mov  ax, [m_x]
    add  ax, 2
    push ax
    push dx
    call SolidAt
    test ax, ax
    jnz  .bump
    mov  ax, [m_x]
    add  ax, 13
    push ax
    push dx
    call SolidAt
    test ax, ax
    jz   .y_done
.bump:
    mov  ax, dx
    shr  ax, 4
    mov  cx, ax                 ; CX = tile row of the block
    inc  ax
    shl  ax, 4
    add  ax, [m_h]
    mov  [m_y], ax              ; head just under the block
    mov  byte [m_yf], 0
    mov  word [m_vy], 0
    mov  ax, [m_x]
    add  ax, 8                  ; tile under the centre of the head
    push ax
    push dx
    call SolidAt
    test ax, ax
    jz   .y_done
    mov  ax, [m_x]
    add  ax, 8
    shr  ax, 4
    push ax
    push cx
    call BlockHit
.y_done:
    popa
    ret

; RectOverlap
;   purpose : test whether two rectangles overlap
;   inputs  : eight words on the stack, [bp+18] = l1 [bp+4] = b2
;   outputs : AX = 1 if they overlap, else 0
;   changes : AX only; removes its 8 parameters (ret 16)

RectOverlap:
    push bp
    mov  bp, sp
    mov  ax, [bp+18]
    cmp  ax, [bp+6]
    jge  .no
    mov  ax, [bp+10]
    cmp  ax, [bp+14]
    jge  .no
    mov  ax, [bp+16]
    cmp  ax, [bp+4]
    jge  .no
    mov  ax, [bp+8]
    cmp  ax, [bp+12]
    jge  .no
    mov  ax, 1
    jmp  .out
.no:
    xor  ax, ax
.out:
    pop  bp
    ret  16

; StartDeath
;   purpose : begin the DYING state 
;   inputs  : [bp+4] = kind: 0 = hop (enemy or time), 1 = pit
;   outputs : state = DYING, death_timer set, Mario stopped (hop speed when kind = 0)
;   changes : none (AX is saved and restored); removes its parameter (ret 2)

StartDeath:
    push bp
    mov  bp, sp
    push ax
    cmp  word [state], ST_PLAYING
    jne  .out
    mov  word [state], ST_DYING
    mov  word [death_timer], DEATH_FRAMES
    mov  byte [m_hp], 0
    mov  byte [m_ground], 0
    mov  word [m_vx], 0
    mov  word [m_vy], 0
    cmp  word [bp+4], 0
    jne  .out
    mov  word [m_vy], DEATH_HOP
.out:
    pop  ax
    pop  bp
    ret  2

; HurtMario
;   purpose : super becomes Small with 2 s invulnerability, Small dies
;   inputs  : none (reads m_invuln, m_form)
;   outputs : Mario shrinks, or the DYING state starts
;   changes : none

HurtMario:
    cmp  byte [m_invuln], 0
    jne  .out
    cmp  byte [m_form], 1
    jne  .die
    mov  byte [m_form], 0
    mov  byte [m_hp], 1
    mov  word [m_h], 16         ; feet stay fixed, only the height changes
    mov  byte [m_invuln], INVULN_FRAMES
    ret
.die:
    push word 0
    call StartDeath
.out:
    ret

; CheckDeathAndFlag
;   purpose : pit death, time-out death, flag pole touch
;   inputs  : none (reads Mario position, m_time, state)
;   outputs : may start DYING, or WIN with the flag points added to the score
;   changes : none (pusha/popa)

CheckDeathAndFlag:
    pusha
    cmp  word [state], ST_PLAYING
    jne  .out
    mov  ax, [m_y]
    sub  ax, [m_h]
    cmp  ax, VIEW_H             ; top of Mario below the bottom of the map
    jl   .time
    push word 1
    call StartDeath
    jmp  .out
.time:
    cmp  word [m_time], 0
    jne  .flag
    push word 0
    call StartDeath
    jmp  .out
.flag:
    mov  ax, [m_x]
    add  ax, 14                 ; one pixel beyond the right edge: touching the pole
    cmp  ax, WORLD_W
    jae  .out                   ; safety: don't query outside the map
    mov  si, ax
    mov  cx, [m_y]
    dec  cx
    push si
    push cx
    call GetTile
    cmp  ax, T_POLE
    je   .touch
    cmp  ax, T_POLE+1
    je   .touch
    mov  cx, [m_y]
    mov  bx, [m_h]
    shr  bx, 1
    sub  cx, bx
    push si
    push cx
    call GetTile
    cmp  ax, T_POLE
    je   .touch
    cmp  ax, T_POLE+1
    je   .touch
    mov  cx, [m_y]
    sub  cx, [m_h]
    push si
    push cx
    call GetTile
    cmp  ax, T_POLE
    je   .touch
    cmp  ax, T_POLE+1
    jne  .out
.touch:
    mov  ax, [m_y]
    dec  ax
    shr  ax, 4                  ; map row of Mario's feet
    cmp  ax, 2
    jae  .row_lo_ok
    mov  ax, 2
.row_lo_ok:
    cmp  ax, 8
    jbe  .row_hi_ok
    mov  ax, 8
.row_hi_ok:
    sub  ax, 2
    shl  ax, 1
    mov  bx, ax
    push word [flag_points+bx]
    call AddScore
    mov  word [state], ST_WIN
    mov  byte [win_phase], 0
    mov  word [m_x], POLE_MARIO_X
    mov  byte [m_xf], 0
    mov  word [m_vx], 0
    mov  word [m_vy], 0
    mov  byte [m_ground], 1
    mov  byte [m_face], 0
    mov  byte [m_invuln], 0
    call ClearHits
.out:
    popa
    ret