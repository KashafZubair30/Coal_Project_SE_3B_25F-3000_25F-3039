SCREEN_W        equ 320
SCREEN_H        equ 200
VIEW_H          equ 192             ; 12 map rows * 16 ; bottom 8 pixel rows stay black
TILE            equ 16
MAP_W           equ 60
MAP_H           equ 12
MAP_SIZE        equ MAP_W*MAP_H
WORLD_W         equ MAP_W*TILE      ; 960
CAM_MAX         equ WORLD_W-SCREEN_W; 640 = last camera position
MARIO_MAX_X     equ WORLD_W-16
VIDEO_SEG       equ 0A000h
FB_PARAS        equ 0FA0h           ; 64000 bytes = 4000 paragraphs
HUD_H           equ 16
GROUND_Y        equ 160             ; feet y when standing on row 9
POLE_X          equ 56*TILE
POLE_MARIO_X    equ POLE_X-8        ; Mario snaps here while sliding
POLE_BASE_Y     equ 9*TILE          ; top of the base block 
CASTLE_X        equ 58*TILE

; colours from the default VGA palette 

COL_BLACK       equ 0
COL_GRAY        equ 8
COL_GREEN       equ 10
COL_RED         equ 12
COL_YELLOW      equ 14
COL_WHITE       equ 15

; tile codes 

T_SKY equ 0
T_GROUND equ 1
T_BRICK equ 2
T_QCOIN equ 3
T_QMUSH equ 4
T_EMPTY equ 5
T_COIN equ 11
T_POLE equ 12                       ; 12 and 13 both count as the flag pole

; scan codes 

SC_ESC equ 01h
SC_P equ 19h
SC_ENTER equ 1Ch
SC_LSHIFT equ 2Ah
SC_SPACE equ 39h
SC_UP equ 48h
SC_LEFT equ 4Bh
SC_RIGHT equ 4Dh
SC_DOWN equ 50h
SC_F1 equ 3Bh
SC_F2 equ 3Ch

; physics, 8.8 fixed point, 30 frames per second 

ACCEL equ 0020h
BRAKE equ 0040h                    
FRICTION equ 0010h
MAX_WALK equ 0180h
MAX_RUN equ 0280h
GRAVITY equ 0040h
JUMP_V equ 0FA80h                   ; -5.5
JUMP_CUT equ 0FE00h                 ; -2.0
MAX_FALL equ 0400h
BOUNCE equ 0FD00h                   ; -3.0
DEATH_HOP equ 0FB1Ah                ; about -4.9 so rises 48 pixels
GOOMBA_SPD equ 0080h
KOOPA_SPD equ 00C0h
SHELL_SPD equ 0300h
MUSH_SPD equ 0100h

; game limits 
START_LIVES equ 3
MAX_LIVES equ 99
LEVEL_TIME equ 300
TICKS_PER_SEC equ 30
PIT_DIVISOR equ 39773               ; 1193182 / 39773 = 30.0 Hz
INVULN_FRAMES equ 60                ; 2 s
DEATH_FRAMES equ 45                 ; 1.5 s
LIFELOST_FRAMES equ 60              ; 2 s
FLAT_FRAMES equ 30                  ; 1 s
KICK_GRACE equ 8
MAX_ACTIVE equ 8
MAX_ENT equ 12
COINPOP_FRAMES equ 12

;  entity record 
E_TYPE equ 0
E_STATE equ 1
E_X equ 2                           ; word, world pixels
E_XF equ 4                          ; byte, fraction
E_YF equ 5                          ; byte, fraction
E_Y equ 6                           ; word, FEET y
E_VY equ 8                          ; word 8.8
E_DIR equ 10                        ; signed byte +1 / -1
E_TIMER equ 11
E_SIZE equ 16
ET_NONE equ 0
ET_GOOMBA equ 1
ET_KOOPA equ 2
ET_MUSH equ 3
ET_COINPOP equ 4
ES_WAIT equ 0
ES_WALK equ 1
ES_FLAT equ 2
ES_SHELL equ 3
ES_SLIDE equ 4

; game states 
ST_TITLE equ 0
ST_PLAYING equ 1
ST_PAUSED equ 2
ST_DYING equ 3
ST_LIFELOST equ 4
ST_WIN equ 5
ST_COMPLETE equ 6
ST_GAMEOVER equ 7
ST_EXIT equ 8
