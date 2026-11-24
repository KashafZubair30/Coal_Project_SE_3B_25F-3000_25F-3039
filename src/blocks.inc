; AddScore
;   purpose : the ONE place where points are added
;   inputs  : [bp+4] = points to add, pushed on the stack
;   outputs : score_hi:score_lo increased (never above 999,999)
;   changes : none (AX is saved); removes its parameter (ret 2)

AddScore:
    push bp
    mov  bp, sp
    push ax
    mov  ax, [bp+4]
    add  [score_lo], ax
    adc  word [score_hi], 0
    cmp  word [score_hi], 0Fh
    jb   .ok
    ja   .clamp
    cmp  word [score_lo], 423Fh
    jbe  .ok
.clamp:
    mov  word [score_hi], 0Fh
    mov  word [score_lo], 423Fh
.ok:
    pop  ax
    pop  bp
    ret  2

; AddLife
;   purpose : +1 life, never above MAX_LIVES (99)
;   inputs  : none
;   outputs : lives + 1
;   changes : none (flags only)

AddLife:
    cmp  byte [lives], MAX_LIVES
    jae  .out
    inc  byte [lives]
.out:
    ret

; AddCoin
;   purpose : coins + 1; the 100th coin gives a life and resets the counter to 0
;   inputs  : none
;   outputs : coins + 1, or coins = 0 and lives + 1
;   changes : none (flags only)

AddCoin:
    inc  byte [coins]
    cmp  byte [coins], 100
    jb   .out
    mov  byte [coins], 0
    call AddLife
.out:
    ret

; BlockHit
;   purpose : Mario's head hit tile (tx, ty): brick, coin block or mushroom block
;   inputs  : [bp+6] = tile column tx, [bp+4] = tile row ty
;   outputs : brick destroyed (+50, Super only), or block emptied with +200 and a coin pop, or block emptied with a mushroom spawned
;   changes : none (pusha/popa); removes its 2 parameters (ret 4)

BlockHit:
    push bp
    mov  bp, sp
    pusha
    mov  ax, [bp+6]
    shl  ax, 4
    mov  bx, [bp+4]
    shl  bx, 4
    push ax
    push bx
    call GetTile
    cmp  al, T_BRICK
    je   .brick
    cmp  al, T_QCOIN
    je   .qcoin
    cmp  al, T_QMUSH
    je   .qmush
    jmp  .done
.brick:
    cmp  byte [m_form], 0
    je   .done                  ; for small mario the brick stays, no points
    push word [bp+6]
    push word [bp+4]
    push word T_SKY
    call SetTile
    push word 50
    call AddScore
    jmp  .done
.qcoin:
    push word [bp+6]
    push word [bp+4]
    push word T_EMPTY
    call SetTile
    push word 200
    call AddScore
    call AddCoin
    push word ET_COINPOP
    push word ES_WALK
    mov  ax, [bp+6]
    shl  ax, 4
    push ax
    mov  ax, [bp+4]
    shl  ax, 4
    push ax
    push word 1
    call SpawnEntity
    test ax, ax
    jz   .done
    mov  bx, ax
    mov  byte [bx+E_TIMER], COINPOP_FRAMES
    jmp  .done
.qmush:
    push word [bp+6]
    push word [bp+4]
    push word T_EMPTY
    call SetTile
    push word ET_MUSH
    push word ES_WALK
    mov  ax, [bp+6]
    shl  ax, 4
    push ax
    mov  ax, [bp+4]
    shl  ax, 4
    push ax                     ; feet = top of the block
    push word 1
    call SpawnEntity
.done:
    popa
    pop  bp
    ret  4

; UpdateClock
;   purpose : one clock unit per TICKS_PER_SEC timer ticks, taken from the tick counter; also counts down the invulnerability time
;   inputs  : none (reads ticks and clock_last)
;   outputs : m_time - 1 per second (never below 0), m_invuln - 1 per frame while above 0
;   changes : none (AX and DX are saved)

UpdateClock:
    push ax
    push dx
    mov  ax, [ticks]
    mov  dx, ax
    sub  ax, [clock_last]
    mov  [clock_last], dx
    add  [clock_acc], ax
.count_second:
    cmp  word [clock_acc], TICKS_PER_SEC
    jb   .invuln
    sub  word [clock_acc], TICKS_PER_SEC
    cmp  word [m_time], 0
    je   .count_second
    dec  word [m_time]
    jmp  .count_second
.invuln:
    cmp  byte [m_invuln], 0
    je   .out
    dec  byte [m_invuln]
.out:
    pop  dx
    pop  ax
    ret

; TryCoin
;   purpose : if the tile at that pixel is a coin: tile becomes sky, +200 points, coins + 1
;   inputs  : [bp+6] = pixel x (world), [bp+4] = pixel y
;   outputs : map, score and coins updated when a coin was there
;   changes : none (AX is saved); removes its 2 parameters (ret 4)

TryCoin:
    push bp
    mov  bp, sp
    push ax
    push word [bp+6]
    push word [bp+4]
    call GetTile
    cmp  ax, T_COIN
    jne  .out
    mov  ax, [bp+6]
    shr  ax, 4
    push ax
    mov  ax, [bp+4]
    shr  ax, 4
    push ax
    push word T_SKY
    call SetTile
    push word 200
    call AddScore
    call AddCoin
.out:
    pop  ax
    pop  bp
    ret  4

; CollectCoins
;   purpose : coin tiles are not solid; collect them when Mario's box overlaps one 
;   inputs  : none (reads m_x, m_y, m_h)
;   outputs : TryCoin called for six points of Mario's hitbox
;   changes : none (pusha/popa)

CollectCoins:
    pusha
    mov  cx, [m_y]
    dec  cx                      ; feet row
    mov  dx, [m_y]
    sub  dx, [m_h]               ; top row
    mov  si, [m_h]
    shr  si, 1
    neg  si
    add  si, [m_y]               ; middle row
    mov  bx, [m_x]
    add  bx, 2                   ; left edge
    mov  di, [m_x]
    add  di, 13                  ; right edge
%macro TRYCOIN 2
    push %1
    push %2
    call TryCoin
%endmacro
    TRYCOIN bx, cx
    TRYCOIN di, cx
    TRYCOIN bx, si
    TRYCOIN di, si
    TRYCOIN bx, dx
    TRYCOIN di, dx
    popa
    ret
