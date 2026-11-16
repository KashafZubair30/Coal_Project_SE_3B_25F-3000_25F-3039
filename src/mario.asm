; mario.asm  -  Super Mario Engine, EL2003 COAL semester project
; Module 1: program skeleton.  Single-segment .COM, ORG 100h.
    cpu 286
    bits 16
    org 100h

%include "constants.inc"=

start:
    cld
    mov  sp, 0FFFEh
    call InitMemory              ; frame buffer segment (INT 21h AH=4Ah / 48h)
    jc   NoMemory
    call BuildArtTables
    call InitVideo
    call InstallKeyboard
    call InstallTimer
    mov  word [state], ST_TITLE
    mov  ax, [ticks]
    mov  [last_tick], ax
    mov  [clock_last], ax

MainLoop:
    call WaitForTick             ; frame step 1: wait for the next timer tick
    mov  bx, [state]
    shl  bx, 1
    call [state_table+bx]        ; jump table indexed by the state number
    cmp  word [state], ST_EXIT
    jne  MainLoop
    jmp  Cleanup                 ; restores vectors, text mode, exits to DOS

NoMemory:
    mov  dx, msg_nomem
    mov  ah, 9
    int  21h
    mov  ax, 4C01h
    int  21h

%include "video.inc"
%include "map.inc"
%include "sprite.inc"
%include "fbuf.inc"
%include "camera.inc"
%include "input.inc"
%include "player.inc"
%include "enemies.inc"
%include "blocks.inc"
%include "hud.inc"
%include "render.inc"
%include "states.inc"
%include "data.inc"
