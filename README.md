# Super Mario Engine  (EL2003 COAL Semester Project, Fall 26)

16-bit x86 platform game in NASM, runs in DOSBox as `MARIO.COM`.

**Team:** Group 01 (SE 3B)
* Kashaf, 25F-3000, modules 1-8 (skeleton and build, video, map and tiles, sprites, frame buffer, camera, keyboard, timer)
* Noor-ul-Eiman, 25F-3039, modules 9-15 (movement, collision, enemies, blocks and coins, score and HUD, game states, win/lose/exit)

**Honesty note:** The assembly code was written by the team. An AI assistant (Claude) was used to review the code against the project manual, to write the procedure comment blocks, and to help with building and testing.

## Build and run
```
nasm -f bin -I src/ src/mario.asm -o MARIO.COM -l build.lst      (or run build.bat)
dosbox MARIO.COM        # in the DOSBox config set  cycles=max  (smooth 30 fps)
```

## Controls
Left/Right arrows move, Left Shift runs, Space jumps (hold = higher), P pause, Esc back to title / exit,
Up/Down + Enter in menus.
**Debug keys (PLAYING only):** F1 sets the clock to 5 s, F2 sets coins to 99.

## Memory choices
The .COM keeps one 64 KB segment (code, data, stack). At start `INT 21h AH=4Ah` shrinks the program block to
64 KB and `AH=48h` allocates 4000 paragraphs (64,000 bytes) for the off-screen frame buffer; its segment is
stored in `fb_seg`. Original map `map_orig` and working copy `map_work` (60x12 bytes); `ResetMap` copies with REP MOVSB.

## Game states (words)
TITLE: Up/Down moves the highlight, Enter starts (new game) or exits, Esc exits. PLAYING: P -> PAUSED,
death -> DYING, flag -> WIN, Esc -> TITLE. PAUSED: picture frozen with a box; P resumes, Esc -> TITLE.
DYING: 1.5 s death animation, lives-1, then LIFE LOST (lives left) or GAME OVER. LIFE LOST: 2 s, level reset,
back to PLAYING. WIN: slide down the pole, walk to the castle, then LEVEL COMPLETE (time x 50 added). LEVEL
COMPLETE and GAME OVER: Enter -> TITLE. EXIT: vectors, PIT rate and text mode restored, return to DOS.
Dispatch is a jump table `state_table` indexed by the state number.

## Modules -> files
1 mario.asm, constants.inc | 2 video.inc | 3 map.inc, level.inc | 4 sprite.inc, art.inc | 5 fbuf.inc |
6 camera.inc | 7, 8 input.inc | 9, 10 player.inc | 11 enemies.inc | 12, 13 blocks.inc, hud.inc |
14, 15 states.inc, render.inc. Data and tables: data.inc.

## Deviations and design notes
* Timer: the PIT is reprogrammed to 30 Hz. The handler counts ticks and chains the ORIGINAL INT 08h only when a
  16-bit accumulator overflows (18.2 times per second) so the DOS clock stays correct; the PIT is restored on exit.
* Art (`art.inc`) and the level map (`level.inc`) are stored as text tables in the source and converted to
  palette indices at start-up with XLAT. Text uses the BIOS ROM 8x8 font (INT 10h AX=1130h), scaled.
* Score events go through `AddScore`; coins and lives through `AddCoin` / `AddLife`.
* Mushroom and popping-coin effects live in the same record table as the enemies.
* Flag test uses the pixel just right of Mario's hitbox, because the pole base block stops him one pixel short.
* Jumping needs a fresh Space press (`m_latch`): holding Space after landing does not start another jump.
* Added visual extras (not in the manual's bonus list, no effect on the rules): parallax clouds that scroll at half
  the camera speed, a coin pop animation above "?" blocks, and a simple castle at the end of the level.

## Bonus features
Walking animation (stand/run frames, Goomba mirror).

## Known limitations
No sound, one level, no high-score file.
