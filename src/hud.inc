
; DrawHUD
;   purpose : draw the black HUD strip (map row 0) with the labels and the values
;             SCORE, COINS, WORLD, TIME, LIVES and the two hearts 
;   inputs  : none (reads score_lo/hi, coins, m_time, lives, m_hp and the table hud_labels)
;   outputs : HUD drawn into the frame buffer
;   changes : none (pusha/popa)

DrawHUD:
    pusha
    push word 0
    push word 0
    push word SCREEN_W
    push word HUD_H
    push word COL_BLACK
    call FillRect
    mov  si, hud_labels          ; table of (x, string) pairs
.label_loop:
    mov  bx, [si]
    test bx, bx
    jz   .values
    push bx
    push word 0
    push word [si+2]
    push word COL_YELLOW
    push word 1
    call DrawText
    add  si, 4
    jmp  .label_loop
.values:
    push word [score_lo]         ; SCORE: 6 digits
    push word [score_hi]
    push word 6
    push word numbuf
    call FormatNum
    push word 4
    push word 8
    push word numbuf
    push word COL_WHITE
    push word 1
    call DrawText
    mov  byte [numbuf], 'x'      ; COINS: x + 2 digits
    mov  al, [coins]
    xor  ah, ah
    push ax
    push word 0
    push word 2
    push word numbuf+1
    call FormatNum
    push word 56
    push word 8
    push word numbuf
    push word COL_WHITE
    push word 1
    call DrawText
    push word 108                ; WORLD
    push word 8
    push word s_world_val
    push word COL_WHITE
    push word 1
    call DrawText
    push word [m_time]           ; TIME: 3 digits
    push word 0
    push word 3
    push word numbuf
    call FormatNum
    push word 160
    push word 8
    push word numbuf
    push word COL_WHITE
    push word 1
    call DrawText
    mov  byte [numbuf], 'x'      ; LIVES
    mov  al, [lives]
    xor  ah, ah
    push ax
    push word 0
    push word 2
    push word numbuf+1
    call FormatNum
    push word 204
    push word 8
    push word numbuf
    push word COL_WHITE
    push word 1
    call DrawText
    mov  byte [numbuf+1], 0      ; HEALTH: heart glyph (code 3) once or twice filled
    mov  byte [numbuf], 3
    push word 256
    push word 8
    push word numbuf
    push word COL_RED
    push word 1
    call DrawText
    mov  ax, COL_GRAY
    cmp  byte [m_hp], 2
    jne  .heart_two
    mov  ax, COL_RED
.heart_two:
    push word 272 
    push word 8
    push word numbuf
    push ax
    push word 1
    call DrawText
    popa
    ret