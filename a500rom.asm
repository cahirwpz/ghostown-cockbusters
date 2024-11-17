        include 'hardware/custom.i'
        include 'hardware/cia.i'
        include 'exec/memory.i'

ChipStart equ $400
ChipEnd   equ $200000
SlowStart equ $c00000
SlowEnd   equ $dc0000
Ghostown  equ $47544e21

; custom chips addresses

custom  equ     $dff000
ciaa    equ     $bfe001
ciab    equ     $bfd000

; 512kB kickstart begin address

        org     $f80000

Kickstart:
        dc.l    $400             ; Initial SP
        dc.l    Entry            ; Initial PC

HunkFilePtr:
        dc.l    0
HunkFileSize:
        dc.l    0

; The ROM is located at $fc0000 but is mapped at $0 after reset shadowing RAM
Entry:
        lea     ciaa,a4
        lea     ciab,a5
        lea     custom,a6
        move.b  #3,ciaddra(a4)  ; Set port A direction to output for /LED and OVL
        move.b  #0,ciapra(a4)   ; Disable OVL (Memory from $0 onwards available)

InitHW:
        move.w  #$7fff,d0       ; Make sure DMA and interrupts are disabled
        move.w  d0,intena(a5)
        move.w  d0,intreq(a5)
        move.w  d0,dmacon(a5)

        ; Wake up line & frame counters
        clr.b   ciatodhi(a4)
        clr.b   ciatodmid(a4)
        clr.b   ciatodlow(a4)
        clr.b   ciatodhi(a5)
        clr.b   ciatodmid(a5)
        clr.b   ciatodlow(a5)

; Chip Memory check
; Output:
;  [d4] Upper limit of chip memory
ChipMem:
        move.l  #Ghostown,d0    ; Our signature
        suba.l  a0,a0
        clr.l   (a0)            ; Write a zero to the first location

.loop   adda.l  #$40000,a0      ; Increment current location by 256KiB
        cmp.l   #ChipEnd,a0
        bgt.s   .exit
        move.l  d0,(a0)         ; Write signature into the memory

        ; Has address decoding wrapped around,
        ; due to incomplete address decoding?
        tst.l   $0.w            ; Was first location overwritten?
        bne.s   .exit
        cmp.l   (a0),d0         ; Is signature still there?
        beq.s   .loop

.exit   move.l  a0,d4

; Slow Memory check
; Output:
;  [d5] Upper limit of expansion memory or zero if missing
SlowMem:
        clr.l   0.w             ; Write a zero to the first location
        lea     SlowStart,a0

.loop   move.l  a0,a1
        lea     $1000(a0),a0              ; Check next 4KiB

        ; If the location written to is INTENA, then INTENAR location
        ; will read zero, because value $3fff disables all interrupts.
        ; In the other case INTENA location was memory, and the value
        ; of INTENAR location is non-determined - could be zero as well.
        move.w  #$3fff,-$1000+intena(a0)
        tst.w   -$1000+intenar(a0)
        bne.s   .memory

        ; If the location written to is INTENA, then INTENAR location
        ; will read $3fff, because value $bfff enables all interrupts
        ; (not really, consider master bit). We know that INTENAR location
        ; was zero, so now it must read as $3fff, if it is not memory.
        move.w  #$bfff,-$1000+intena(a0)
        cmp.w   #$3fff,-$1000+intenar(a0)
        beq.s   .exit

.memory move.l  d0,(a1)         ; Write signature into the memory

        ; Has address decoding wrapped around,
        ; due to incomplete address decoding?
        tst.l   $0.w            ; Was first location overwritten?
        bne.s   .exit
        cmp.l   (a1),d0         ; Is signature still there?
        bne.s   .exit

        cmp.l   #SlowEnd,a0
        blt.s   .loop

.exit   ; Just in case disable interrupts
        move.w  #$7fff,-$1000+intenar(a0)
        move.l  a1,a0
        cmp.l   #SlowStart,a0
        bne.s   .done
        suba.l  a0,a0

.done   move.l  a0,d5

; This prepares memory and registers to jump into Start
; It needs to set up d2-d3/a3/a6/sp
;
; [d4] The end of CHIP memory
Setup:
        move.l  HunkFileSize(pc),d2
        move.l  HunkFilePtr(pc),d3
        move.l  #BD_SIZE+2*MR_SIZE,a3
        sub.l   a3,sp
        move.l  sp,a6

        clr.l   BD_HUNK(a6)
        clr.l   BD_VBR(a6)
        move.b  #1,BD_BOOTDEV(a6)
        clr.b   BD_CPUMODEL(a6)
        clr.w   BD_NREGIONS(a6)

        lea     BD_REGION(a6),a0
        tst.l   d5
        beq.s   .chip

.slow   move.l  #SlowStart,(a0)+
        move.l  d5,(a0)+
        move.w  #MEMF_FAST|MEMF_PUBLIC,(a0)+
        add.w   #1,BD_NREGIONS(a6)

.chip   move.l  #ChipStart,(a0)+
        move.l  d4,(a0)+
        move.w  #MEMF_CHIP|MEMF_PUBLIC,(a0)+
        add.w   #1,BD_NREGIONS(a6)

ROM = 1

        include 'bootloader.asm'

; vim: ft=asm68k:ts=8:sw=8:noet:
