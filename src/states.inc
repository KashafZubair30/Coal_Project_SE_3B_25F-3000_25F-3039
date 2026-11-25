; ResetLevel
;   purpose : put map, enemies, hero, camera and clock back to the level start
;   inputs  : none
;   outputs : none (map, enemy table, hero, cam_x, m_time, clock variables reset)
;   changes : no registers (AX is saved and restored)

ResetLevel:
    push ax
    call ResetMap
    call ResetEnemies
    call ResetHero
    mov  word [cam_x], 0
    mov  word [m_time], LEVEL_TIME
    mov  word [clock_acc], 0
    mov  ax, [ticks]
    mov  [clock_last], ax
    call ClearHits
    pop  ax
    ret

; NewGame
;   purpose : start from scratch: score, coins, lives, then the level
;   inputs  : none
;   outputs : none (score = 0, coins = 0, lives = START_LIVES)
;   changes : no registers (ResetLevel preserves them)

NewGame:
    mov  word [score_lo], 0
    mov  word [score_hi], 0
    mov  byte [coins], 0
    mov  byte [lives], START_LIVES
    call ResetLevel
    ret

; SetState
;   purpose : change the game state and forget pending key presses
;   inputs  : [bp+4] = new state number (ST_ constant), pushed on the stack
;   outputs : state = newstate
;   changes : no registers (AX is saved and restored); removes its parameter (ret 2)

SetState:
    push bp
    mov  bp, sp
    push ax
    mov  ax, [bp+4]
    mov  [state], ax
    call ClearHits
    pop  ax
    pop  bp
    ret  2

; StTitle
;   purpose : TITLE state, one frame: menu keys, then draw the title screen
;   inputs  : none (reads the key table through TestHit)
;   outputs : may change state (PLAYING or EXIT) and menu_sel; draws and shows one frame
;   changes : no registers (pusha/popa)

StTitle:
    pusha
    push word SC_UP
    call TestHit
    test ax, ax
    jz   .chk_down
    mov  byte [menu_sel], 0
.chk_down:
    push word SC_DOWN
    call TestHit
    test ax, ax
    jz   .chk_esc
    mov  byte [menu_sel], 1
.chk_esc:
    push word SC_ESC
    call TestHit
    test ax, ax
    jz   .chk_enter
    push word ST_EXIT
    call SetState
    jmp  .draw
.chk_enter:
    push word SC_ENTER
    call TestHit
    test ax, ax
    jz   .draw
    cmp  byte [menu_sel], 0
    jne  .exit
    call NewGame
    push word ST_PLAYING
    call SetState
    jmp  .done
.exit:
    push word ST_EXIT
    call SetState
.draw:
    call DrawTitleScreen
    call FlipScreen
.done:
    popa
    ret

; DrawTitleScreen
;   purpose : draw the title screen into the frame buffer
;   inputs  : none (reads menu_sel)
;   outputs : frame buffer filled with the title screen
;   changes : no registers (pusha/popa)

DrawTitleScreen:
    pusha
    call ClearFrame
    xor  si, si
.top:
    mov  ax, si
    shl  ax, 4
    push ax
    push word 0
    push word T_BRICK
    call DrawTile
    push ax
    push word 176
    push word T_BRICK
    call DrawTile
    inc  si
    cmp  si, 20
    jb   .top
    mov  si, 1
.side:
    mov  ax, si
    shl  ax, 4
    push word 0
    push ax
    push word T_BRICK
    call DrawTile
    push word 304
    push ax
    push word T_BRICK
    call DrawTile
    inc  si
    cmp  si, 11
    jb   .side
    mov  si, 1
.ground:
    mov  ax, si
    shl  ax, 4
    push ax
    push word 160
    push word T_GROUND
    call DrawTile
    inc  si
    cmp  si, 19
    jb   .ground
    push word 48
    push word 144
    push word spr_small_stand
    push word 0
    call DrawSprite
    push word 256
    push word 144
    push word spr_goomba
    push word 0
    call DrawSprite
    push word 26                 ; title with a black drop shadow
    push word s_title
    push word COL_BLACK
    push word 3
    call DrawTextC
    push word 24
    push word s_title
    push word COL_RED
    push word 3
    call DrawTextC
    push word 62
    push word s_engine
    push word COL_WHITE
    push word 2
    call DrawTextC
    push word 66                 ; menu box
    push word 92
    push word 188
    push word 56
    push word COL_WHITE
    call FillRect
    push word 68
    push word 94
    push word 184
    push word 52
    push word COL_BLACK
    call FillRect
    push word 88
    push word 100
    push word s_start
    push word COL_YELLOW
    push word 2
    call DrawText
    push word 88
    push word 122
    push word s_exit
    push word COL_YELLOW
    push word 2
    call DrawText
    mov  ax, 100                 ; highlight arrow
    cmp  byte [menu_sel], 0
    je   .arrow
    mov  ax, 122
.arrow:
    push word 70
    push ax
    push word s_arrow
    push word COL_YELLOW
    push word 2
    call DrawText
    push word 0
    push word VIEW_H
    push word SCREEN_W
    push word SCREEN_H-VIEW_H
    push word COL_BLACK
    call FillRect
    push word 166
    push word s_team
    push word COL_WHITE
    push word 1
    call DrawTextC
    popa
    ret

; StPlaying
;   purpose : PLAYING state, one frame
;   inputs  : none
;   outputs : may change state (PAUSED or TITLE); updates and draws the world
;   changes : no registers (pusha/popa)

StPlaying:
    pusha
    push word SC_P
    call TestHit
    test ax, ax
    jz   .noP
    push word ST_PAUSED
    call SetState
    jmp  .done
.noP:
    push word SC_ESC
    call TestHit
    test ax, ax
    jz   .noEsc
    push word ST_TITLE
    call SetState
    jmp  .done
.noEsc:
    call UpdateMario             ; frame order
    call UpdateEntities
    call ResolveEntities
    call CollectCoins
    call UpdateClock
    call CheckDeathAndFlag
    call UpdateCamera
    call RenderWorld
.done:
    popa
    ret

; StPaused
;   purpose : PAUSED state, one frame: P resumes, Esc goes to the title, else draw the pause box
;   inputs  : none
;   outputs : may change state (PLAYING or TITLE); shows one frame
;   changes : no registers (pusha/popa)

StPaused:
    pusha
    push word SC_P
    call TestHit
    test ax, ax
    jz   .noP
    mov  ax, [ticks]
    mov  [clock_last], ax        ; do not count the paused time
    push word ST_PLAYING
    call SetState
    jmp  .done
.noP:
    push word SC_ESC
    call TestHit
    test ax, ax
    jz   .draw
    push word ST_TITLE
    call SetState
    jmp  .done
.draw:
    push word 80
    push word 66
    push word 160
    push word 52
    push word COL_WHITE
    call FillRect
    push word 82
    push word 68
    push word 156
    push word 48
    push word COL_BLACK
    call FillRect
    push word 76
    push word s_paused
    push word COL_YELLOW
    push word 2
    call DrawTextC
    push word 100
    push word s_resume
    push word COL_WHITE
    push word 1
    call DrawTextC
    call FlipScreen
.done:
    popa
    ret

; StDying
;   purpose : DYING state, one frame: gravity-only fall, then lose a life when the timer ends
;   inputs  : none (reads death_timer, lives)
;   outputs : may change state (LIFELOST or GAMEOVER); lives decreases by 1 at the end
;   changes : no registers (pusha/popa)

StDying:
    pusha
    mov  ax, [m_vy]              ; gravity only, no collisions
    add  ax, GRAVITY
    cmp  ax, MAX_FALL
    jle  .ok
    mov  ax, MAX_FALL
.ok:
    mov  [m_vy], ax
    mov  dx, ax
    add  [m_yf], dl
    mov  al, dh
    cbw
    adc  [m_y], ax
    call RenderWorld
    dec  word [death_timer]
    jnz  .out
    dec  byte [lives]
    jz   .over
    mov  word [state_timer], LIFELOST_FRAMES
    push word ST_LIFELOST
    call SetState
    jmp  .done
.over:
    push word ST_GAMEOVER
    call SetState
.out:
.done:
    popa
    ret

; StLifeLost
;   purpose : LIFE LOST screen, one frame: show lives left, then restart the level
;   inputs  : none (reads state_timer, lives)
;   outputs : shows one frame; when the timer ends the level is reset and state = PLAYING
;   changes : no registers (pusha/popa)

StLifeLost:
    pusha
    push word 0
    push word 0
    push word SCREEN_W
    push word SCREEN_H
    push word COL_BLACK
    call FillRect
    push word 40
    push word s_worldname
    push word COL_WHITE
    push word 3
    call DrawTextC
    push word 108
    push word 88
    push word spr_small_stand
    push word 0
    call DrawSprite
    mov  byte [numbuf], 'x'
    mov  al, [lives]
    xor  ah, ah
    push ax
    push word 0
    push word 1
    push word numbuf+1
    call FormatNum
    push word 150
    push word 88
    push word numbuf
    push word COL_WHITE
    push word 3
    call DrawText
    push word 140
    push word s_liveslft
    push word COL_YELLOW
    push word 2
    call DrawTextC
    call FlipScreen
    dec  word [state_timer]
    jnz  .out
    call ResetLevel
    push word ST_PLAYING
    call SetState
.out:
.done:
    popa
    ret

; StWin
;   purpose : WIN sequence, one frame: slide down the pole, walk to the castle, add time x 50
;   inputs  : none (reads win_phase, state_timer, m_time)
;   outputs : may change state to COMPLETE and add the time bonus to the score
;   changes : no registers (pusha/popa)

StWin:
    pusha
    cmp  byte [win_phase], 0
    jne  .walk
    add  word [m_y], 3           ; slide down the pole
    cmp  word [m_y], POLE_BASE_Y
    jl   .render
    mov  word [m_y], POLE_BASE_Y
    mov  byte [win_phase], 1
    mov  word [state_timer], 32
    jmp  .render
.walk:
    mov  word [m_y], GROUND_Y
    mov  word [m_vx], 0100h      ; non-zero: run pose
    add  word [m_x], 2           ; walk towards the castle
    dec  word [state_timer]
    jnz  .render
    call RenderWorld
    mov  ax, [score_lo]          ; Level Complete: remember the score, add time x 50
    mov  [lc_pre_lo], ax
    mov  ax, [score_hi]
    mov  [lc_pre_hi], ax
    mov  ax, [m_time]
    mov  cx, 50
    mul  cx                      ; DX:AX = time * 50; max 300*50=15000 fits in AX
    mov  [lc_bonus], ax
    push ax
    call AddScore
    push word ST_COMPLETE
    call SetState
    jmp  .done
.render:
    call UpdateCamera
    call RenderWorld
.done:
    popa
    ret

; StComplete
;   purpose : LEVEL COMPLETE screen, one frame: show the score table, Enter returns to the title
;   inputs  : none
;   outputs : may change state to TITLE; shows one frame
;   changes : no registers (pusha/popa)

StComplete:
    pusha
    push word SC_ENTER
    call TestHit
    test ax, ax
    jz   .draw
    push word ST_TITLE
    call SetState
    jmp  .done
.draw:
    call ClearFrame
    push word 0
    push word 0
    push word SCREEN_W
    push word SCREEN_H
    push word COL_BLACK
    call FillRect
    call DrawBorder14
    push word 24
    push word s_complete
    push word COL_YELLOW
    push word 2
    call DrawTextC

    ; labels

    push word 56
    push word 60
    push word s_lbl_score
    push word COL_WHITE
    push word 1
    call DrawText
    push word 56
    push word 76
    push word s_lbl_coins
    push word COL_WHITE
    push word 1
    call DrawText
    push word 56
    push word 92
    push word s_lbl_time
    push word COL_WHITE
    push word 1
    call DrawText
    push word 56
    push word 108
    push word s_lbl_bonus
    push word COL_WHITE
    push word 1
    call DrawText
    push word 56
    push word 124
    push word s_lbl_final
    push word COL_WHITE
    push word 1
    call DrawText

    ; values

    push word [lc_pre_lo]
    push word [lc_pre_hi]
    push word 6
    push word numbuf
    call FormatNum
    push word 176
    push word 60
    push word numbuf
    push word COL_WHITE
    push word 1
    call DrawText
    mov  byte [numbuf], 'x'
    mov  al, [coins]
    xor  ah, ah
    push ax
    push word 0
    push word 2
    push word numbuf+1
    call FormatNum
    push word 176
    push word 76
    push word numbuf
    push word COL_WHITE
    push word 1
    call DrawText
    push word [m_time]
    push word 0
    push word 3
    push word numbuf
    call FormatNum
    push word 176
    push word 92
    push word numbuf
    push word COL_WHITE
    push word 1
    call DrawText
    push word 208
    push word 92
    push word s_x50
    push word COL_WHITE
    push word 1
    call DrawText
    mov  byte [numbuf], '+'
    push word [lc_bonus]
    push word 0
    push word 5
    push word numbuf+1
    call FormatNum
    push word 176
    push word 108
    push word numbuf
    push word COL_WHITE
    push word 1
    call DrawText
    push word [score_lo]
    push word [score_hi]
    push word 6
    push word numbuf
    call FormatNum
    push word 176
    push word 124
    push word numbuf
    push word COL_YELLOW
    push word 1
    call DrawText
    push word 160
    push word s_enter
    push word COL_GREEN
    push word 1
    call DrawTextC
    call FlipScreen
.done:
    popa
    ret

; DrawBorder14
;   purpose : draw the yellow border of the end screens
;   inputs  : none
;   outputs : border drawn into the frame buffer
;   changes : none

DrawBorder14:
    push word 8
    push word 8
    push word 304
    push word 3
    push word COL_YELLOW
    call FillRect
    push word 8
    push word 181
    push word 304
    push word 3
    push word COL_YELLOW
    call FillRect
    push word 8
    push word 8
    push word 3
    push word 176
    push word COL_YELLOW
    call FillRect
    push word 309
    push word 8
    push word 3
    push word 176
    push word COL_YELLOW
    call FillRect
    ret

; StGameOver
;   purpose : GAME OVER screen, one frame: show the final score, Enter returns to the title
;   inputs  : none
;   outputs : may change state to TITLE; shows one frame
;   changes : no registers (pusha/popa)

StGameOver:
    pusha
    push word SC_ENTER
    call TestHit
    test ax, ax
    jz   .draw
    push word ST_TITLE
    call SetState
    jmp  .done
.draw:
    push word 0
    push word 0
    push word SCREEN_W
    push word SCREEN_H
    push word COL_BLACK
    call FillRect
    push word 24
    push word 24
    push word 272
    push word 3
    push word COL_RED
    call FillRect
    push word 24
    push word 165
    push word 272
    push word 3
    push word COL_RED
    call FillRect
    push word 24
    push word 24
    push word 3
    push word 144
    push word COL_RED
    call FillRect
    push word 293
    push word 24
    push word 3
    push word 144
    push word COL_RED
    call FillRect
    push word 48
    push word s_gameover
    push word COL_RED
    push word 3
    call DrawTextC
    push word [score_lo]
    push word [score_hi]
    push word 6
    push word numbuf
    call FormatNum
    push word 90
    push word s_final
    push word COL_WHITE
    push word 1
    call DrawTextC
    push word 104
    push word numbuf
    push word COL_YELLOW
    push word 2
    call DrawTextC
    mov  byte [numbuf], 0
    push word 130
    push word s_enter_title
    push word COL_GREEN
    push word 1
    call DrawTextC
    call FlipScreen
.done:
    popa
    ret