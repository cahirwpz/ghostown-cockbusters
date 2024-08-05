        include 'exec/types.i'

        xdef    _DrawTriPart

WIDTH           equ     128
WIDTH_BITS      equ     7

 STRUCTURE SIDE,0               ; Keep in sync with SideT from texobj.c !
        WORD    DX
        WORD    DY
        WORD    DU
        WORD    DV
        WORD    DXDY
        WORD    DUDY
        WORD    DVDY
        WORD    X
        WORD    U
        WORD    V
        WORD    YS
        WORD    YE
        LABEL   SIDE_SIZE

        section ".text"

; [a0] chunky
; [a1] texture (const)
; [a2] left (const)
; [a3] right (const)
; [d2] du (----UUuu)
; [d3] dv (----VVvv)
; [d4] ys
; [d5] ye
_DrawTriPart:
        movem.l d2-d5/a4,-(sp)

        ; height must be positive
        sub.w   d4,d5
        subq.w  #1,d5
        bmi     .quit

        ; line = chunky + ys * WIDTH
        lsl.w   #WIDTH_BITS,d4
        lea     (a0,d4.w),a4

; in:
;  [d2] u (----UUuu)
;  [d3] v (----VVvv)
; out:
;  [d2] step (------UU)
;  [d3] step (uu--VVvv)

        swap    d3      ; VVvv----
        move.w  d2,d3   ; VVvvUUuu
        rol.w   #8,d3   ; VVvvuuUU
        clr.l   d2      ; --------
        move.b  d3,d2   ; ------UU
        clr.b   d3      ; VVvvuu--
        swap    d3      ; uu--VVvv

.loop:
        ; xs = (l->x + 127) >> 8
        move.w  X(a2),d0
        add.w   #127,d0
        asr.w   #8,d0

        ; xe = (r->x + 127) >> 8
        move.w  X(a3),d4
        add.w   #127,d4
        asr.w   #8,d4

        ; xs >= xe => skip
        cmp.w   d4,d0
        bge     .skip

        ; pixels = &line[xs]
        lea     (a4,d0.w),a0
        
        ; n = xe - xs
        sub.w   d0,d4

        ; u = l->u
        move.w  U(a2),d0

        ; v = l->v
        move.w  V(a2),d1

; in:
;  [d0] u (----UUuu)
;  [d1] v (----VVvv)
; out:
;  [d0] sum (------UU)
;  [d1] sum (uu--VVvv)

        swap    d1      ; VVvv----
        move.w  d0,d1   ; VVvvUUuu
        rol.w   #8,d1   ; VVvvuuUU
        clr.l   d0      ; --------
        move.b  d1,d0   ; ------UU
        clr.b   d1      ; VVvvuu--
        swap    d1      ; uu--VVvv
        
; jump into unrolled loop

        sub.w   #WIDTH,d4
        neg.w   d4
        mulu.w  #14,d4  ; 14 is the size of single iteration
        jmp     .start(pc,d4.w)

; Texturing code inspired by article by Kalms
; https://amycoders.org/opt/innerloops.html
.start:
        rept    WIDTH
        move.w  d1,d4           ;   [4] ----VVvv
        add.l   d3,d1           ;   [8] uu--VVvv 
        move.b  d0,d4           ;   [4] ----VVUU
        addx.b  d2,d0           ;   [4] ------UU
        add.b   d4,d4           ;   [4]
        move.b  (a1,d4.w),(a0)+ ;  [18]
        endr                    ; [=42]
        
.skip:  ; l->x += l->dxdy
        move.w  DXDY(a2),d4
        add.w   d4,X(a2)
        
        ; l->u += l->dudy
        move.w  DUDY(a2),d4
        add.w   d4,U(a2)

        ; l->v += l->dvdy
        move.w  DVDY(a2),d4
        add.w   d4,V(a2)

        ; r->x += r->dxdy
        move.w  DXDY(a3),d4
        add.w   d4,X(a3)
        
        ; r->u += r->dudy
        move.w  DUDY(a3),d4
        add.w   d4,U(a3)

        ; r->v += r->dvdy
        move.w  DVDY(a3),d4
        add.w   d4,V(a3)

        ; line += WIDTH
        lea     WIDTH(a4),a4
        dbf     d5,.loop

.quit:  movem.l (sp)+,d2-d5/a4
        rts
