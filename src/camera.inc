; UpdateCamera
;   purpose : once per frame after Mario moved, and keep Mario
;             from leaving through the left edge of the screen
;   inputs  : none (reads m_x and cam_x)
;   outputs : cam_x updated; m_x, m_xf and m_vx changed when Mario hits the left wall
;   changes : none (AX is saved and restored)

UpdateCamera:
    push ax
    mov  ax, [m_x]
    sub  ax, [cam_x]            ; screen_x
    cmp  ax, 160
    jle  .wall
    mov  ax, [m_x]
    sub  ax, 160                ; new_camera
    cmp  ax, CAM_MAX
    jbe  .nomax
    mov  ax, CAM_MAX
.nomax:
    cmp  ax, [cam_x]
    jbe  .wall                  ; max(camera, new_camera): never decrease
    mov  [cam_x], ax
.wall:
    mov  ax, [m_x]
    cmp  ax, [cam_x]
    jge  .done
    mov  ax, [cam_x]            ; Mario may not leave through the left edge
    mov  [m_x], ax
    mov  byte [m_xf], 0
    mov  word [m_vx], 0
.done:
    pop  ax
    ret
