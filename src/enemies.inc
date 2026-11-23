; ResetEnemies
;   purpose : copy the start table enemy_init over the live table 
;   inputs  : none
;   outputs : entities = enemy_init
;   changes : none (CX, SI, DI, ES are saved and restored)

ResetEnemies:
    push cx
    push si
    push di
    push es
    push ds
    pop  es
    cld
    mov  si, enemy_init
    mov  di, entities
    mov  cx, MAX_ENT*E_SIZE
    rep  movsb
    pop  es
    pop  di
    pop  si
    pop  cx
    ret

; SpawnEntity
;   purpose : use the first free record of the entity table
;   inputs  : [bp+12] type, [bp+10] state, [bp+8] x, [bp+6] y, [bp+4] dir
;   outputs : AX = address of the new record, or 0 when the table is full
;   changes : AX only (BX and CX are saved); removes its 5 parameters (ret 10)

SpawnEntity:
    push bp
    mov  bp, sp
    push bx
    push cx
    mov  bx, entities
    mov  cx, MAX_ENT
.find:
    cmp  byte [bx+E_TYPE], 0
    je   .got
    add  bx, E_SIZE
    loop .find
    xor  ax, ax
    jmp  .out
.got:
    mov  al, [bp+12]
    mov  [bx+E_TYPE], al
    mov  al, [bp+10]
    mov  [bx+E_STATE], al
    mov  ax, [bp+8]
    mov  [bx+E_X], ax
    mov  ax, [bp+6]
    mov  [bx+E_Y], ax
    mov  al, [bp+4]
    mov  [bx+E_DIR], al
    xor  ax, ax
    mov  [bx+E_VY], ax
    mov  [bx+E_XF], al
    mov  [bx+E_YF], al
    mov  [bx+E_TIMER], al
    mov  ax, bx
.out:
    pop  cx
    pop  bx
    pop  bp
    ret  10

; CountActive
;   purpose : count the active Goombas and Koopas 
;   inputs  : none
;   outputs : AX = number of active Goombas / Koopas
;   changes : AX only (BX, CX, DX are saved)

CountActive:
    push bx
    push cx
    push dx
    xor  dx, dx
    mov  bx, entities
    mov  cx, MAX_ENT
.loop:
    mov  al, [bx+E_TYPE]
    cmp  al, ET_GOOMBA
    je   .check_state
    cmp  al, ET_KOOPA
    jne  .skip
.check_state:
    mov  al, [bx+E_STATE]
    cmp  al, ES_WALK
    jb   .skip
    cmp  al, ES_FLAT
    je   .skip
    inc  dx
.skip:
    add  bx, E_SIZE
    loop .loop
    mov  ax, dx
    pop  dx
    pop  cx
    pop  bx
    ret

; FindOverlap
;   purpose : find another active Goomba/Koopa touching this record 
;   inputs  : [bp+4] = address of this record
;   outputs : AX = address of the other record, or 0
;   changes : AX only (BX, CX, SI are saved); removes its parameter (ret 2)

FindOverlap:
    push bp
    mov  bp, sp
    push bx
    push cx
    push si
    mov  si, [bp+4]
    mov  bx, entities
    mov  cx, MAX_ENT
.loop:
    cmp  bx, si
    je   .next
    mov  al, [bx+E_TYPE]
    cmp  al, ET_GOOMBA
    je   .typeok
    cmp  al, ET_KOOPA
    jne  .next
.typeok:
    mov  al, [bx+E_STATE]
    cmp  al, ES_WALK
    jb   .next
    cmp  al, ES_FLAT
    je   .next
    mov  ax, [bx+E_X]
    sub  ax, [si+E_X]
    jns  .dx_abs
    neg  ax
.dx_abs:
    cmp  ax, 12
    jae  .next
    mov  ax, [bx+E_Y]
    sub  ax, [si+E_Y]
    jns  .dy_abs
    neg  ax
.dy_abs:
    cmp  ax, 12
    jae  .next
    mov  ax, bx
    jmp  .out
.next:
    add  bx, E_SIZE
    loop .loop
    xor  ax, ax
.out:
    pop  si
    pop  cx
    pop  bx
    pop  bp
    ret  2

; UpdateEntities
;   purpose : step 4 of the frame: update every record of the entity table
;   inputs  : none
;   outputs : all entities updated
;   changes : none (CX and SI are saved)

UpdateEntities:
    push cx
    push si
    mov  si, entities
    mov  cx, MAX_ENT
.loop:
    push si
    call UpdateOneEntity
    add  si, E_SIZE
    loop .loop
    pop  si
    pop  cx
    ret

; UpdateOneEntity
;   purpose : activation, walking, turning, falling and shell kills for one record
;   inputs  : [bp+4] = address of the record
;   outputs : the record is moved, turned, removed or changed; may add score
;   changes : none (pusha/popa); removes its parameter (ret 2)

UpdateOneEntity:
    push bp
    mov  bp, sp
    pusha
    mov  bx, [bp+4]
    mov  al, [bx+E_TYPE]
    test al, al
    jz   .ret
    cmp  al, ET_COINPOP
    jne  .notpop
    sub  word [bx+E_Y], 3       ; coin popping out of a block
    dec  byte [bx+E_TIMER]
    jnz  .ret
    mov  byte [bx+E_TYPE], 0
    jmp  .ret
.notpop:
    cmp  byte [bx+E_STATE], ES_WAIT
    jne  .active
    mov  ax, [bx+E_X]           ; wakes up within one tile of the screen
    sub  ax, [cam_x]
    cmp  ax, SCREEN_W+TILE
    jge  .ret
    call CountActive
    cmp  ax, MAX_ACTIVE
    jae  .ret
    mov  byte [bx+E_STATE], ES_WALK
    jmp  .ret
.active:
    mov  ax, [bx+E_X]
    sub  ax, [cam_x]
    cmp  ax, -32
    jl   .remove                ; far left of the camera
    cmp  word [bx+E_Y], VIEW_H+TILE
    jg   .remove                ; fell below the map
    cmp  byte [bx+E_STATE], ES_FLAT
    jne  .alive
    dec  byte [bx+E_TIMER]
    jnz  .ret
.remove:
    mov  byte [bx+E_TYPE], 0
    jmp  .ret
.alive:
    cmp  byte [bx+E_TIMER], 0
    je   .speed
    dec  byte [bx+E_TIMER]      ; kick grace
.speed:
    xor  ax, ax
    mov  dl, [bx+E_STATE]
    cmp  dl, ES_SHELL
    je   .gotspeed              ; stationary shell: no walking
    mov  ax, SHELL_SPD
    cmp  dl, ES_SLIDE
    je   .gotspeed
    mov  ax, GOOMBA_SPD
    cmp  byte [bx+E_TYPE], ET_GOOMBA
    je   .gotspeed
    mov  ax, KOOPA_SPD
    cmp  byte [bx+E_TYPE], ET_KOOPA
    je   .gotspeed
    mov  ax, MUSH_SPD
.gotspeed:
    test ax, ax
    jz   .vertical
    cmp  byte [bx+E_DIR], 0
    jg   .dirpos
    neg  ax
.dirpos:
    mov  dx, ax
    add  [bx+E_XF], dl         
    mov  al, dh
    cbw
    adc  [bx+E_X], ax
    mov  ax, [bx+E_X]
    cmp  byte [bx+E_DIR], 0
    jg   .lead_right
    add  ax, 2
    jmp  .gotlead
.lead_right:
    add  ax, 13
.gotlead:
    mov  dx, ax                 ; DX = leading edge
    mov  cx, [bx+E_Y]
    dec  cx
    push dx
    push cx
    call SolidAt
    test ax, ax
    jnz  .wall
    mov  cx, [bx+E_Y]
    sub  cx, 8
    push dx
    push cx
    call SolidAt
    test ax, ax
    jz   .edge
.wall:
    cmp  byte [bx+E_DIR], 0
    jg   .wallr
    mov  ax, dx
    shr  ax, 4
    inc  ax
    shl  ax, 4
    sub  ax, 2
    mov  [bx+E_X], ax
    mov  byte [bx+E_DIR], 1
    jmp  .wallfix
.wallr:
    mov  ax, dx
    shr  ax, 4
    shl  ax, 4
    sub  ax, 14
    mov  [bx+E_X], ax
    mov  byte [bx+E_DIR], 0FFh
.wallfix:
    mov  byte [bx+E_XF], 0
    jmp  .vertical
.edge:
    cmp  byte [bx+E_TYPE], ET_KOOPA     ; Koopas also turn at platform edges
    jne  .others
    cmp  byte [bx+E_STATE], ES_WALK
    jne  .others
    cmp  word [bx+E_VY], 0100h
    jge  .others
    push dx
    push word [bx+E_Y]
    call SolidAt
    test ax, ax
    jnz  .others
    neg  byte [bx+E_DIR]
    jmp  .vertical
.others:
    cmp  byte [bx+E_STATE], ES_SLIDE
    je   .shellkill
    cmp  byte [bx+E_TYPE], ET_MUSH
    je   .vertical
    cmp  byte [bx+E_STATE], ES_WALK
    jne  .vertical
    push bx                     ; walkers turn around when they bump into another enemy
    call FindOverlap
    test ax, ax
    jz   .vertical
    mov  si, ax
    mov  ax, [si+E_X]
    cmp  byte [bx+E_DIR], 0
    jg   .facing_r
    cmp  ax, [bx+E_X]
    jge  .vertical             
    mov  byte [bx+E_DIR], 1
    jmp  .vertical
.facing_r:
    cmp  ax, [bx+E_X]
    jle  .vertical
    mov  byte [bx+E_DIR], 0FFh
    jmp  .vertical
.shellkill:
    push bx
    call FindOverlap
    test ax, ax
    jz   .vertical
    mov  si, ax
    mov  byte [si+E_TYPE], 0    ; the sliding shell defeats it
    push word 500
    call AddScore
.vertical:
    mov  ax, [bx+E_VY]
    add  ax, GRAVITY
    cmp  ax, MAX_FALL
    jle  .vyok
    mov  ax, MAX_FALL
.vyok:
    mov  [bx+E_VY], ax
    mov  dx, ax
    add  [bx+E_YF], dl
    mov  al, dh
    cbw
    adc  [bx+E_Y], ax
    cmp  word [bx+E_VY], 0
    jle  .ret
    mov  ax, [bx+E_X]
    add  ax, 2
    push ax
    push word [bx+E_Y]
    call SolidAt
    test ax, ax
    jnz  .land
    mov  ax, [bx+E_X]
    add  ax, 13
    push ax
    push word [bx+E_Y]
    call SolidAt
    test ax, ax
    jz   .ret
.land:
    mov  ax, [bx+E_Y]
    shr  ax, 4
    shl  ax, 4
    mov  [bx+E_Y], ax
    mov  byte [bx+E_YF], 0
    mov  word [bx+E_VY], 0
.ret:
    popa
    pop  bp
    ret  2

; KickShell
;   purpose : +400 points; the shell slides away from Mario
;   inputs  : [bp+4] = address of the shell record
;   outputs : shell state = SLIDE, direction away from Mario
;   changes : none (AX and BX are saved); removes its parameter (ret 2)

KickShell:
    push bp
    mov  bp, sp
    push ax
    push bx
    mov  bx, [bp+4]
    push word 400
    call AddScore
    mov  byte [bx+E_STATE], ES_SLIDE
    mov  byte [bx+E_TIMER], KICK_GRACE
    mov  ax, [bx+E_X]
    cmp  ax, [m_x]
    jl   .left
    mov  byte [bx+E_DIR], 1
    jmp  .out
.left:
    mov  byte [bx+E_DIR], 0FFh
.out:
    pop  bx
    pop  ax
    pop  bp
    ret  2

; StompScore
;   purpose : stomp chain points from chain_table; the chain caps at the 10th stomp (8000 points)
;   inputs  : none (reads m_chain)
;   outputs : score increased, m_chain + 1 (stops at 255)
;   changes : none (AX and BX are saved)

StompScore:
    push ax
    push bx
    mov  al, [m_chain]
    cmp  al, 10
    jae  .inc                   ; chain cap reached, no more points
    xor  ah, ah
    shl  ax, 1
    mov  bx, ax
    push word [chain_table+bx]
    call AddScore
.inc:
    cmp  byte [m_chain], 255
    je   .out
    inc  byte [m_chain]
.out:
    pop  bx
    pop  ax
    ret

; HitEnemy
;   purpose : when Mario touches a Goomba, Koopa or shell
;   inputs  : [bp+4] = address of the enemy record
;   outputs : stomp: flat or shell, points, Mario bounces; otherwise kick or HurtMario
;   changes : none (pusha/popa); removes its parameter (ret 2)

HitEnemy:
    push bp
    mov  bp, sp
    pusha
    mov  bx, [bp+4]
    cmp  word [m_vy], 0
    jle  .nostomp              
    mov  ax, [bx+E_Y]
    sub  ax, 8
    cmp  [m_prev_y], ax
    jg   .nostomp               ; and feet above the upper half last frame
    mov  byte [m_stomp_flag], 1
    mov  al, [bx+E_STATE]
    cmp  al, ES_SHELL
    je   .kick
    cmp  al, ES_SLIDE
    je   .stopshell
    cmp  byte [bx+E_TYPE], ET_GOOMBA
    je   .flatten
    mov  byte [bx+E_STATE], ES_SHELL    ; Koopa becomes a stationary shell
    mov  byte [bx+E_TIMER], KICK_GRACE
    call StompScore
    jmp  .bounce
.flatten:
    mov  byte [bx+E_STATE], ES_FLAT
    mov  byte [bx+E_TIMER], FLAT_FRAMES
    call StompScore
    jmp  .bounce
.stopshell:
    mov  byte [bx+E_STATE], ES_SHELL
    mov  byte [bx+E_TIMER], KICK_GRACE
    call StompScore
    jmp  .bounce
.kick:
    cmp  byte [bx+E_TIMER], 0
    jne  .done
    push bx
    call KickShell
.bounce:
    mov  word [m_vy], BOUNCE
    mov  byte [m_ground], 0
    jmp  .done
.nostomp:
    mov  al, [bx+E_STATE]
    cmp  al, ES_SHELL
    jne  .hostile
    cmp  byte [bx+E_TIMER], 0
    jne  .done                  ; shell ignores Mario during its grace time
    push bx
    call KickShell
    jmp  .done
.hostile:
    cmp  al, ES_SLIDE
    jne  .hurt
    cmp  byte [bx+E_TIMER], 0
    jne  .done
.hurt:
    cmp  byte [m_stomp_flag], 0
    jne  .done
    call HurtMario
.done:
    popa
    pop  bp
    ret  2

; HitMushroom
;   purpose : +1000 points, Small becomes Super (HP 2), mushroom removed
;   inputs  : [bp+4] = address of the mushroom record
;   outputs : record removed; m_form, m_hp and m_h updated
;   changes : none (BX is saved); removes its parameter (ret 2)

HitMushroom:
    push bp
    mov  bp, sp
    push bx
    mov  bx, [bp+4]
    mov  byte [bx+E_TYPE], 0
    push word 1000
    call AddScore
    cmp  byte [m_form], 0
    jne  .out
    mov  byte [m_form], 1
    mov  byte [m_hp], 2
    mov  word [m_h], 32         ; feet fixed: top = y_feet - 32
.out:
    pop  bx
    pop  bp
    ret  2

; ResolveEntities
;   purpose : Mario against enemies and items with RectOverlap
;   inputs  : none (reads Mario variables and the entity table)
;   outputs : calls HitEnemy / HitMushroom for every touching record
;   changes : none (pusha/popa)

ResolveEntities:
    pusha
    mov  byte [m_stomp_flag], 0
    mov  si, entities
    mov  cx, MAX_ENT
.loop:
    cmp  word [state], ST_PLAYING
    jne  .done
    mov  al, [si+E_TYPE]
    test al, al
    jz   .next
    cmp  al, ET_COINPOP
    je   .next
    mov  ah, [si+E_STATE]
    cmp  ah, ES_WAIT
    je   .next
    cmp  ah, ES_FLAT
    je   .next
    mov  ax, [m_x]
    add  ax, 2
    push ax                     ; l1
    mov  ax, [m_y]
    sub  ax, [m_h]
    push ax                     ; t1
    mov  ax, [m_x]
    add  ax, 14
    push ax                     ; r1
    push word [m_y]             ; b1
    mov  ax, [si+E_X]
    add  ax, 2
    push ax                     ; l2
    mov  ax, [si+E_Y]
    sub  ax, 16
    push ax                     ; t2
    mov  ax, [si+E_X]
    add  ax, 14
    push ax                     ; r2
    push word [si+E_Y]          ; b2
    call RectOverlap
    test ax, ax
    jz   .next
    push si
    cmp  byte [si+E_TYPE], ET_MUSH
    je   .mush
    call HitEnemy
    jmp  .next
.mush:
    call HitMushroom
.next:
    add  si, E_SIZE
    loop .loop
.done:
    popa
    ret