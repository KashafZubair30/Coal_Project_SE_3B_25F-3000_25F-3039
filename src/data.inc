; hero
m_x dw 0
m_xf db 0
m_y dw 0                         ; FEET
m_yf db 0
m_prev_y dw 0
m_vx dw 0
m_vy dw 0
m_h dw 16
m_form db 0                      ; 0 small, 1 super
m_hp db 1
m_ground db 1
m_invuln db 0
m_chain db 0
m_face db 0                      ; 0 right, 1 left
m_latch db 0
m_stomp_flag db 0

; score / clock
score_lo dw 0
score_hi dw 0
coins db 0
lives db START_LIVES
m_time dw LEVEL_TIME
clock_acc dw 0
clock_last dw 0
cam_x dw 0

;  timer, keyboard
ticks dw 0
tick_acc dw 0
last_tick dw 0
frame_ctr dw 0
kb_ext db 0
old_int8 dw 0, 0                 ; offset, segment
old_int9 dw 0, 0
keys times 128 db 0
keys_hit times 128 db 0

;  state machine
state dw ST_TITLE
menu_sel db 0
state_timer dw 0
death_timer dw 0
win_phase db 0
lc_pre_lo dw 0
lc_pre_hi dw 0
lc_bonus dw 0

;  video
fb_seg dw 0
font_off dw 0
font_seg dw 0
chartab times 256 db 0
numbuf times 12 db 0

; tables
state_table dw StTitle, StPlaying, StPaused, StDying, StLifeLost, StWin, StComplete, StGameOver, StGameOver
chain_table dw 100, 200, 400, 500, 800, 1000, 2000, 4000, 5000, 8000
flag_points dw 5000, 5000, 2000, 800, 400, 400, 100      ; feet row 2..8
cloud_table dw 40,24, 230,40, 430,24, 640,48, 860,28, 1100,40, 0
hud_labels dw 4,s_score, 56,s_coins, 108,s_world, 160,s_time, 204,s_lives, 256,s_health, 0

%macro ENEMY 2                   ; type, tile column
    db %1, ES_WAIT
    dw %2*16
    db 0, 0
    dw GROUND_Y
    dw 0
    db 0FFh, 0
    times 4 db 0

%endmacro
enemy_init:
    ENEMY ET_GOOMBA, 15
    ENEMY ET_GOOMBA, 19
    ENEMY ET_GOOMBA, 26
    ENEMY ET_KOOPA, 31
    ENEMY ET_GOOMBA, 43
    ENEMY ET_KOOPA, 47
    times (MAX_ENT-6)*E_SIZE db 0
entities times MAX_ENT*E_SIZE db 0
map_work times MAP_SIZE db 0

;  strings
s_score db 'SCORE',0
s_coins db 'COINS',0
s_world db 'WORLD',0
s_world_val db '1-1',0
s_time db 'TIME',0
s_lives db 'LIVES',0
s_health db 'HEALTH',0
s_title db 'SUPER MARIO',0
s_engine db 'E N G I N E',0
s_start db 'START GAME',0
s_exit db 'EXIT',0
s_arrow db '>',0
s_team db 'Group 01 | Kashaf, Noor Ul Eiman',0
s_paused db 'PAUSED',0
s_resume db 'Press P to resume',0
s_worldname db 'WORLD 1-1',0
s_liveslft db 'LIVES LEFT',0
s_complete db 'LEVEL COMPLETE!',0
s_lbl_score db 'SCORE',0
s_lbl_coins db 'COINS',0
s_lbl_time db 'TIME LEFT',0
s_lbl_bonus db 'TIME BONUS',0
s_lbl_final db 'FINAL SCORE',0
s_x50 db 'x 50',0
s_enter db 'Press ENTER to continue',0
s_gameover db 'GAME OVER',0
s_final db 'FINAL SCORE',0
s_enter_title db 'Press ENTER for the title',0
msg_nomem db 'Not enough memory for the frame buffer.',13,10,'$'
%include "art.inc"
