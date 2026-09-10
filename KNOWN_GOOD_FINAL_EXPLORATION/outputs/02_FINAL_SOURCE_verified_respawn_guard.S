.syntax unified
.cpu arm7tdmi
.thumb
.section .text,"ax",%progbits
.global start
.type start,%function
.thumb_func
start:
/* 0x000: Audit Fix 1 safe PLAYER_NORMAL predicate */
safe_player:
    push {r4,lr}
    adds r4,r0,#0
    ldr r1, =0x03001160
    cmp r4,r1
    bne safe_no
    ldrb r2,[r4,#0x0c]
    cmp r2,#1
    bne safe_no
    ldr r2, =0x030010A0
    ldrb r3,[r2,#8]
    cmp r3,#0
    bne safe_no
    /* camera_target is not a player-state invariant. Fixed-camera rooms and
       camera managers may use NULL/another entity during ordinary walking. */
    ldr r2, =0x03003F80
    ldrb r3,[r2,#2]
    cmp r3,#0
    bne safe_no
    movs r3,#0x26
    ldrb r3,[r2,r3]
    cmp r3,#0
    bne safe_no
    ldrb r3,[r2,#0x0c]
    cmp r3,#0
    bne safe_no
    movs r3,#0x8b
    ldrb r3,[r2,r3]
    cmp r3,#0
    bne safe_no
    movs r3,#0x42
    ldrb r3,[r4,r3]
    cmp r3,#0
    bne safe_no
    ldrh r3,[r4,#0x36]
    cmp r3,#0
    bne safe_no
    ldr r3,[r2,#0x30]
    ldr r2, =0x351C1637
    tst r3,r2
    bne safe_no
    ldrb r3,[r4,#0x15]
    movs r2,#0x80
    tst r3,r2
    bne safe_no
    movs r0,#1
    pop {r4,pc}
safe_no:
    movs r0,#0
    pop {r4,pc}

.org 0x64
/* 0x064: movement wrapper. Entry address must stay 0203F065. */
movement_wrapper:
    push {r4,lr}
    adds r4,r0,#0
    bl safe_player
    cmp r0,#0
    beq movement_original
    movs r1,#0
    strh r1,[r4,#0x2a]
    ldrh r1,[r4,#0x24]
    ldrb r2,[r4,#0x15]
    adds r0,r4,#0
    ldr r3, =0x080027EB
    bl call_r3
    pop {r4,pc}
movement_original:
    adds r0,r4,#0
    ldr r3, =0x0800857D
    bl call_r3
    pop {r4,pc}

.org 0x8c
/* 0x08C: Audit Fix 1 room/exploration wrapper. Entry must stay 0203F08D. */
exploration_wrapper:
    push {r4,r5,r6,r7,lr}
    sub sp,#12
    adds r7,r0,#0
    ldr r4, =0x03001160
    adds r0,r4,#0
    bl safe_player
    cmp r0,#0
    bne ew_safe
    adds r0,r7,#0
    ldr r3, =0x0807A5B9
    bl call_r3
    b ew_done
ew_safe:
    ldr r5, =0x03000BF0
    ldrh r0,[r4,#0x2e]
    ldrh r1,[r4,#0x32]
    str r0,[sp,#4]
    str r1,[sp,#8]
    ldrh r2,[r5,#6]
    cmp r2,#0
    bne ew_xlo_check
    ldr r3, =0x0000F000
    cmp r0,r3
    bhi ew_left
ew_xlo_check:
    cmp r0,r2
    blo ew_left
    ldrh r3,[r5,#0x1e]
    adds r3,r3,r2
    cmp r0,r3
    bhs ew_right
    ldrh r2,[r5,#8]
    cmp r2,#0
    bne ew_ylo_check
    ldr r3, =0x0000F000
    cmp r1,r3
    bhi ew_up
ew_ylo_check:
    cmp r1,r2
    blo ew_up
    ldrh r3,[r5,#0x20]
    adds r3,r3,r2
    cmp r1,r3
    bhs ew_down
    b ew_done
ew_up:    movs r7,#0; b ew_dir
ew_right: movs r7,#1; b ew_dir
ew_down:  movs r7,#2; b ew_dir
ew_left:  movs r7,#3
ew_dir:
    str r7,[sp]
    /* First priority: real BORDER/BORDER2 transition. */
    cmp r7,#1
    beq ew_lr
    cmp r7,#3
    beq ew_lr
    ldr r0,[sp,#4]
    ldrh r2,[r5,#6]
    subs r0,r0,r2
    ldrh r2,[r5,#0x1e]
    cmp r0,r2
    blo 1f
    subs r0,r2,#1
1:  cmp r7,#0
    beq 2f
    ldrh r1,[r5,#0x20]
    b ew_real_exit
2:  movs r1,#0
    b ew_real_exit
ew_lr:
    ldr r1,[sp,#8]
    ldrh r2,[r5,#8]
    subs r1,r1,r2
    ldrh r2,[r5,#0x20]
    cmp r1,r2
    blo 3f
    subs r1,r2,#1
3:  cmp r7,#3
    beq 4f
    ldrh r0,[r5,#0x1e]
    b ew_real_exit
4:  movs r0,#0
ew_real_exit:
    adds r2,r7,#0
    movs r3,#5
    ldr r6, =0x080806BD
    bl call_r6
    cmp r0,#0
    bne ew_done
    /* Second priority: Nintendo same-area resolver. */
    ldr r0, =0x03001160
    adds r1,r7,#0
    ldr r6, =0x0807BD15
    bl call_r6
    cmp r0,#0
    bne ew_done
    /* Third: after 16 px OOB, compact global fallback. */
    ldr r0,[sp,#4]
    ldr r1,[sp,#8]
    ldr r7,[sp]
    cmp r7,#3
    beq ew_oob_left
    cmp r7,#1
    beq ew_oob_right
    cmp r7,#0
    beq ew_oob_up
    ldrh r2,[r5,#8]
    ldrh r3,[r5,#0x20]
    adds r2,r2,r3
    subs r2,r1,r2
    b ew_oob_test
ew_oob_left:
    ldrh r2,[r5,#6]
    cmp r2,#0
    bne 5f
    ldr r3, =0x0000F000
    cmp r0,r3
    bhi ew_global
5:  subs r2,r2,r0
    b ew_oob_test
ew_oob_right:
    ldrh r2,[r5,#6]
    ldrh r3,[r5,#0x1e]
    adds r2,r2,r3
    subs r2,r0,r2
    b ew_oob_test
ew_oob_up:
    ldrh r2,[r5,#8]
    cmp r2,#0
    bne 6f
    ldr r3, =0x0000F000
    cmp r1,r3
    bhi ew_global
6:  subs r2,r2,r1
ew_oob_test:
    cmp r2,#0x10
    blo ew_done
ew_global:
    /* ALttP-style rule: artificial global room traversal only indoors.
       Minish Cap's normal overworld areas are below 0x20; interiors/dungeons
       use the interior area bank beginning at 0x20. Overworld still keeps
       wall no-clip, real exits and Nintendo same-area scrolling. */
    ldrb r1,[r5,#4]
    cmp r1,#0x20
    blo ew_done
    adds r0,r7,#0
    bl global_fallback
ew_done:
    add sp,#12
    pop {r4,r5,r6,r7,pc}

/* Compact runtime RoomHeader enumerator. r0 = direction 0/1/2/3. */
global_fallback:
    push {r4,r5,r6,r7,lr}
    sub sp,#8
    str r0,[sp]              /* direction */
    ldr r4, =0x03000BF0     /* RoomControls */
    movs r5,#0              /* valid room count */
    movs r6,#0xff           /* current ordinal, FF = not found */
    movs r7,#0x20           /* interior area bank only */
/* pass 1: count valid rooms and locate current room */
gf_count_area:
    cmp r7,#0x90
    bhs gf_count_done
    ldr r2, =0x0811E214
    lsls r3,r7,#2
    ldr r0,[r2,r3]
    cmp r0,#0
    beq gf_count_next_area
    movs r1,#0
gf_count_room:
    ldrh r2,[r0]
    adds r2,#1
    lsls r2,r2,#16
    beq gf_count_next_area   /* x == FFFF => area terminator */
    /* USA interior table audit: no pre-terminator header has y == FFFF;
       width/height remain the validity filter. */
    ldrh r2,[r0,#4]
    cmp r2,#0
    beq gf_count_skip
    ldrh r2,[r0,#6]
    cmp r2,#0
    beq gf_count_skip
    ldrb r2,[r4,#4]
    cmp r7,r2
    bne gf_count_valid
    ldrb r2,[r4,#5]
    cmp r1,r2
    bne gf_count_valid
    adds r6,r5,#0
gf_count_valid:
    adds r5,#1
gf_count_skip:
    adds r0,#10
    adds r1,#1
    cmp r1,#0x40
    blo gf_count_room
gf_count_next_area:
    adds r7,#1
    b gf_count_area
gf_count_done:
    cmp r6,#0xff
    bne 14f
    b gf_fail
14:
    ldr r7,[sp]            /* direction */
    cmp r7,#1
    beq gf_right
    cmp r7,#3
    beq gf_left
    cmp r7,#2
    beq gf_down
/* up = 16 predecessor steps with wrap */
    movs r0,#16
gf_up_loop:
    subs r6,#1
    bpl 7f
    adds r6,r5,#0
7:
    subs r0,#1
    bne gf_up_loop
    b gf_target_ready
gf_right:
    adds r6,#1
    cmp r6,r5
    blo gf_target_ready
    movs r6,#0
    b gf_target_ready
gf_left:
    cmp r6,#0
    bne 8f
    adds r6,r5,#0
8:  subs r6,#1
    b gf_target_ready
gf_down:
    movs r0,#16
gf_down_loop:
    adds r6,#1
    cmp r6,r5
    blo 9f
    movs r6,#0
9:  subs r0,#1
    bne gf_down_loop
gf_target_ready:
    str r6,[sp,#4]          /* target ordinal */

/* pass 2: resolve ordinal to area, room, RoomHeader */
    movs r5,#0
    movs r7,#0x20
gf_find_area:
    /* Pass 1 proved target ordinal < valid count; with the identical filter,
       pass 2 must resolve before the area limit. */
    ldr r2, =0x0811E214
    lsls r3,r7,#2
    ldr r0,[r2,r3]
    cmp r0,#0
    beq gf_find_next_area
    movs r1,#0
gf_find_room:
    ldrh r2,[r0]
    adds r2,#1
    lsls r2,r2,#16
    beq gf_find_next_area
    ldrh r2,[r0,#4]
    cmp r2,#0
    beq gf_find_skip
    ldrh r2,[r0,#6]
    cmp r2,#0
    beq gf_find_skip
    ldr r2,[sp,#4]
    cmp r5,r2
    beq gf_found
    adds r5,#1
gf_find_skip:
    adds r0,#10
    adds r1,#1
    cmp r1,#0x40
    blo gf_find_room
gf_find_next_area:
    adds r7,#1
    b gf_find_area

gf_found:
    /* r7=area, r1=room, r0=target header */
    str r1,[sp,#4]          /* reuse as target room */
    adds r6,r0,#0           /* target header */
    ldr r5, =0x03001160
    ldr r0,[sp]             /* direction */
    cmp r0,#1
    beq gf_spawn_right
    cmp r0,#3
    beq gf_spawn_left
    cmp r0,#0
    beq gf_spawn_up
/* DOWN: preserve current local X exactly when possible, enter 8 px from top. */
    ldrh r0,[r5,#0x2e]
    ldrh r1,[r4,#6]
    subs r0,r0,r1
    ldrh r1,[r6,#4]
    bl clamp_axis
    movs r1,#8
    b gf_commit
gf_spawn_up:
    ldrh r0,[r5,#0x2e]
    ldrh r1,[r4,#6]
    subs r0,r0,r1
    ldrh r1,[r6,#4]
    bl clamp_axis
    ldrh r1,[r6,#6]
    cmp r1,#16
    blo 10f
    subs r1,#8
    b gf_commit
10: lsrs r1,r1,#1
    b gf_commit
gf_spawn_right:
    ldrh r0,[r5,#0x32]
    ldrh r1,[r4,#8]
    subs r0,r0,r1
    ldrh r1,[r6,#6]
    bl clamp_axis
    adds r1,r0,#0
    movs r0,#8
    b gf_commit
gf_spawn_left:
    ldrh r0,[r5,#0x32]
    ldrh r1,[r4,#8]
    subs r0,r0,r1
    ldrh r1,[r6,#6]
    bl clamp_axis
    adds r1,r0,#0
    ldrh r0,[r6,#4]
    cmp r0,#16
    blo 11f
    subs r0,#8
    b gf_commit
11: lsrs r0,r0,#1

gf_commit:
    ldr r6, =0x030010A0
    movs r2,#1
    strb r2,[r6,#8]
    strb r2,[r6,#9]
    strb r7,[r6,#0x0c]
    ldr r3,[sp,#4]
    strb r3,[r6,#0x0d]
    ldr r3,[sp]
    lsls r3,r3,#1
    strb r3,[r6,#0x0e]
    movs r2,#0
    strb r2,[r6,#0x0f]
    strh r0,[r6,#0x10]
    strh r1,[r6,#0x12]
    strb r2,[r6,#0x14]     /* let TMC resolve collision layer */
gf_fail:
    add sp,#8
    pop {r4,r5,r6,r7,pc}

/* Preserve the orthogonal room-relative coordinate. Only clamp when the
   destination room is genuinely smaller than the source coordinate. */
clamp_axis:
    cmp r1,#0
    beq clamp_zero
    cmp r0,r1
    blo 13f
    adds r0,r1,#0
    subs r0,#1
13: bx lr
clamp_zero:
    movs r0,#0
    bx lr

/* The build also replaces only the PLAYER_NORMAL call at 08070B98 to
   sub_08008AC6 with two Thumb NOPs.  That confirmed call path is the
   collision-stuck safety respawn which otherwise reaches RespawnPlayer ->
   ResetPlayerPosition.  The second sub_08008AC6 call used by special player
   handling remains original, so roll/jump/swim/knockback/cutscene paths are
   not globally disabled. */

/* Exact castle/cliff test addendum: conveyer_push at 08074BF8 sets the normal
   surface lockout fields and its LinearMoveUpdate call at 08074C38 reaches
   coordinate stores 0806F6D8/0806F700, undoing one no-clip walking step.  The
   build replaces only the helper entry with BX LR.  Its four callers already
   accept only the normal non-Minish surface path; jump, roll, swim, knockback,
   climbing, transitions and scripts remain outside it. */

/* Explicit grounded void walking: the build replaces SurfaceAction_Pit entry
   08074200 with BX LR.  That routine is the exact normal surface dispatcher
   which queues PLAYER_FALL for sky void and ordinary chasm tiles.  Airborne
   movement and other special actions remain handled by their original paths. */

/* ARMv4T register-call trampolines, preserving LR from BL. */
call_r3:
    bx r3
call_r6:
    bx r6
    nop

.ltorg
.size start, .-start
