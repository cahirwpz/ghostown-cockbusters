        include 'hardware/custom.i'
        include 'hardware/cia.i'
        include 'exec/memory.i'
        include 'exec/execbase.i'

        ; AttnFlags for 68060
        BITDEF  AF,68060,7

        ; CACR for 68060
        BITDEF  CACR,EIC,15             ; Enable Instruction Cache

ChipStart       equ $400
ChipEnd         equ $200000
SlowStart       equ $c00000
SlowEnd         equ $dc0000
AutoConfigZ2    equ $e80000
AutoConfigZ3    equ $ff000000
MemSpaceZ2      equ $200000
MemSpaceZ3      equ $40000000

Ghostown  equ $47544e21

; custom chips addresses

custom  equ     $dff000
ciaa    equ     $bfe001
ciab    equ     $bfd000

; 512kB kickstart begin address

        org     $f80000

Kickstart:
        dc.l    ChipStart        ; Initial SP
        dc.l    Entry            ; Initial PC

HunkFilePtr:
        dc.l    0
HunkFileSize:
        dc.l    0

; Breaks directly into fs-uae debugger, requires stack to work
BREAK   MACRO
        pea.l   15
        jsr     $f0ff60
ENDM

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

InitIrq:
        lea     $0.w,a0
        move.l  #ChipStart,(a0)+
        move.l  #Entry,(a0)+

        lea     .handler(pc),a1
        move.w  #$2f-2,d0
.loop:  move.l  a1,(a0)+
        dbf     d0,.loop

        bra.s   ChipMem

.handler:
        stop    #$2700
        illegal

; Chip Memory check
; Output:
;  [d4] Size of chip memory
ChipMem:
        lea     ChipStart,a0
        lea     ChipEnd,a1
        move.l  #$400,d0
        bsr     MemoryTest
        move.l  d0,d4

; Slow Memory check
; Output:
;  [d5] Size of expansion memory or zero if missing
SlowMem:
        lea     SlowStart,a0
        lea     SlowEnd,a1
        move.l  #$400,d0
        bsr     MemoryTest
        move.l  d0,d5

; This prepares memory and registers to jump into Start
; It needs to set up d2-d3/a3/a6/sp
;
; [d4] The size of chip memory
; [d5] The size of expansion memory
Setup:
        move.l  HunkFileSize(pc),d2
        move.l  HunkFilePtr(pc),d3
        move.l  #BD_SIZE+3*MR_SIZE,a3
        sub.l   a3,sp
        move.l  sp,a6

        clr.l   BD_HUNK(a6)
        clr.l   BD_VBR(a6)
        move.b  #1,BD_BOOTDEV(a6)
        clr.b   BD_CPUMODEL(a6)
        clr.w   BD_NREGIONS(a6)

        lea     BD_REGION(a6),a2

        lea     AutoConfigZ2,a0
        lea     MemSpaceZ2,a1
        bsr     AutoConfig

        lea     AutoConfigZ3,a0
        lea     MemSpaceZ3,a1
        bsr     AutoConfig

.slow   tst.l   d5
        beq.s   .chip

        move.l  #SlowStart,(a2)+
        add.l   #SlowStart,d5
        move.l  d5,(a2)+
        move.w  #MEMF_FAST|MEMF_PUBLIC,(a2)+
        add.w   #1,BD_NREGIONS(a6)

.chip   move.l  #ChipStart,(a2)+
        move.l  d4,(a2)+
        move.w  #MEMF_CHIP|MEMF_PUBLIC,(a2)+
        add.w   #1,BD_NREGIONS(a6)

        bsr     CpuDetect
        move.l  d0,d1

ROM = 1

        include 'bootloader.asm'

; [a0] board
; [d0] register offset
ReadZorro:
        movem.l d1/d2/a1,-(sp)

        move.l  a0,d2
        and.l   #AutoConfigZ3,d2
        beq.s   .lo_z2
.lo_z3  lea     $100(a0),a1
        move.b  (a1,d0.w),d1    ; read lo nibble (Zorro3)
        bra.s   .hi
.lo_z2  move.b  $2(a0,d0.w),d1  ; read lo nibble (Zorro2)

.hi     move.b  (a0,d0.w),d0    ; read hi nibble
        and.b   #$f0,d0
        lsr.b   #4,d1
        or.b    d1,d0

        movem.l (sp)+,d1/d2/a1
        rts

; [a0] board
; [d0] register offset
; [d1] byte
WriteZorro:
        movem.l a0/d0/d2,-(sp)

        lea     (a0,d0.w),a0    ; register base

        move.b  d1,d0
        lsl.b   #4,d1

        move.l  a0,d2
        and.l   #AutoConfigZ3,d2
        beq.s   .lo_z2
.lo_z3  move.b  d1,$100(a0)      ; write lo nibble (Zorro3)
        bra.s   .hi
.lo_z2  move.b  d1,$2(a0)        ; write lo nibble (Zorro2)

.hi     move.b  d0,(a0)         ; write hi nibble

        movem.l (sp)+,a0/d0/d2
        rts

; Zorro registers
ER_TYPE         equ     $00
ER_FLAGS        equ     $08
EC_Z3_HIGHBASE  equ     $44
EC_BASEADDRESS  equ     $48
EC_SHUTUP       equ     $4c

; ER_TYPE register
ERT_TYPEMASK    equ     $c0
ERT_ZORROII     equ     $c0
ERT_ZORROIII    equ     $80
ERT_MEMLIST     equ     $20
; Zorro2: (0)8MiB (1)64KiB (2)128KiB (3)256KiB (4)512KiB (5)1MiB (6)2MiB (7)4MiB
; Zorro3: (0)16MiB (1)32MiB (2)64MiB (3)128MiB (4)256MiB (5)512MiB (6)1GiB (7)reserved
ERT_MEMMASK     equ     $07

; ER_FLAGS register
ERFB_EXTENDED   equ     5
ERFB_ZORRO_III  equ     4

; Zorro II/III AutoConfig Memory check
AutoConfig:
        movem.l d2-d3,-(sp)

        moveq.l #ER_TYPE,d0
        bsr     ReadZorro
        move.l  d0,d2

        moveq.l #ER_FLAGS,d0
        bsr     ReadZorro
        not.b   d0              ; flags are negated!
        move.l  d0,d3

        cmp.b   #$ff,d2
        beq.s   .nodev

        move.b  d2,d1
        and.b   #ERT_TYPEMASK,d1
        beq.s   .nodev

        move.b  d2,d1
        and.b   #ERT_MEMLIST,d1
        beq.s   .nodev

        ; Is it Zorro II device?
        btst    #ERFB_ZORRO_III,d3
        beq.s   .z2size

        ; Extract memory size bits
        move.b  d2,d0
        and.w   #ERT_MEMMASK,d0

        ; How size should be interpreted? (extended / unextended)
        btst    #ERFB_EXTENDED,d3
        beq.s   .z2size

.z3size move.l  #1<<24,d1
        lsl.l   d0,d1
        bra.s   .chunk

.z2size subq.w  #1,d0
        and.w   #ERT_MEMMASK,d0
        move.l  #$10000,d1
        lsl.l   d0,d1

.chunk  move.l  a1,(a2)+
        add.l   a1,d1
        move.l  d1,(a2)+
        move.w  #MEMF_FAST|MEMF_PUBLIC,(a2)+
        add.w   #1,BD_NREGIONS(a6)

        ; determine A31-A16 of physical address to map the device
        move.l  a1,d1
        swap    d1

        ; write it to Zorro II/III base address register
        move.b  d2,d0
        and.b   #ERT_TYPEMASK,d0
        cmp.b   #ERT_ZORROII,d0
        beq.s   .z2base

.z3base move.b  d1,EC_BASEADDRESS(a0)
        move.w  d1,EC_Z3_HIGHBASE(a0)
        bra.s   .shutup

.z2base moveq.l #EC_BASEADDRESS,d0
        bsr     WriteZorro

.shutup moveq.l #EC_SHUTUP,d0
        moveq.l #0,d1
        bsr     WriteZorro

.nodev  movem.l (sp)+,d2-d3
        rts

; Modified version of arch/m68k-amiga/expansion/memorytest.S
; from AROS repository.
;
; IN: A0 - Address, A1 - Max end address, D0 = block size
; OUT: D0 - Detected size

MemoryTest:
        movem.l d2-d5/a2-a3,-(sp)

        move.l  d0,d5
        move.l  a1,d0
        sub.l   a0,d0                   ; max size

        move.l  a0,d1
        and.l   #$ff000000,d1
        beq.s   .24bitaddr
        ; test if 32bit address mirrors address zero
        move.l  d1,a1
        move.l  0.w,d2                  ; save old
        move.l  $100.w,d3
        move.l  #$fecaf00d,d1
        move.l  d1,0.w
        nop
        not.w   d1
        move.l  d1,$100.w               ; write something else, some bus types "remember" old value
        not.w   d1
        nop                             ; force 68040/060 bus cycle to finish
        cmp.l   (a1),d1
        bne.s   .32bitok                ; different? no mirror
        move.l  #$cafed00d,d1
        move.l  d1,0.w
        nop
        not.w   d1
        move.l  d1,$100.w
        not.w   d1
        nop
        cmp.l   (a1),d1
        bne.s   .32bitok                ; check again, maybe 0 already had our test value
        move.l  d2,0.w                  ; restore saved value
        move.l  d3,$100.w
        moveq   #-1,d1
        bra     .done                   ; 24-bit CPU, do not test this range
.32bitok:
        move.l  d2,0.w                  ; restore saved value
        move.l  d3,$100.w
.24bitaddr:

        ; a0 = tested address, d0 = max size, d1 = current size

        clr.l   d1
.loop:
        cmp.l   d0,d1
        bge     .done

        move.l  a0,d2
        and.l   #$ff000000,d2
        bne.s   .chipcheck_done         ; no chiptest if 32bit address
        move.w  #$7fff,custom+intena
        nop
        tst.w   intenar(a0,d1.l)        ; If non-zero, this is not INTENAR
        bne.s   .chipcheck_done
        ; It was zero ...
        move.w  #$c000,custom+intena    ; Try the master enable
        nop
        tst.w   intenar(a0,d1.l)        ; If still zero, not INTENAR
        bne     .done                   ; It was a custom chip.
.chipcheck_done:

        move.l  a0,a2
        add.l   d1,a2
        cmp.l   #MemoryTest,a2          ; Make sure we don't modify our own test code
        bcs.s   .nottestcode
        cmp.l   #.end,a2
        bcs.s   .next
.nottestcode:

        move.l  (a0,d1.l),d3            ; read old value
        move.l  (a0),a2                 ; save mirror test contents
        move.l  #$fecaf00d,(a0)         ; write mirror test value
        nop
        move.l  #$cafed00d,d2
        move.l  d2,(a0,d1.l)            ; write test pattern
        nop
        tst.l   d1                      ; first test addrress?
        beq.s   .nomirror
        cmp.l   (a0),d2                 ; no, check mirrorirng
        bne.s   .nomirror
        move.l  a2,(a0)                 ; restore mirror test contents
        bra.s   .done
.nomirror:

        not.l   d2
        move.l  4(a0,d1.l),a3           ; read temp address
        move.l  d2,4(a0,d1.l)           ; fill bus with something else
        not.l   d2
        nop
        move.l  (a0,d1.l),d4            ; read test pattern
        move.l  a3,4(a0,d1.l)           ; restore

        cmp.l   d4,d2                   ; pattern match?
        bne.s   .done
        neg.l   d2                      ; test pattern 2

        move.l  d2,(a0,d1.l)            ; write test pattern
        nop
        not.l   d2
        move.l  4(a0,d1.l),a3           ; read temp address
        move.l  d2,4(a0,d1.l)           ; fill bus with something else
        not.l   d2
        nop
        move.l  (a0,d1.l),d4            ; read test pattern
        move.l  a3,4(a0,d1.l)           ; restore

        cmp.l   d4,d2
        bne.s   .done
        not.l   d2
        move.l  d3,(a0,d1.l)            ; write old value back

        move.l  a2,(a0)                 ; restore mirror test contents
.next:
        add.l   d5,d1                   ; next block
        bra     .loop

.done:
        move.l  d1,d0
        movem.l (sp)+,d2-d5/a2-a3
        rts
.end

; Based on arch/m68k-amiga/boot/cpu_detect.S from AROS repository.
; OUT: D0 - Detected CPU/FPU model (same as AttnFlags in ExecBase)

        mc68060

CpuDetect:
        move.l  d2,-(sp)

        moveq.l #0,d0                   ; detected CPU/FPU model
        move.l  sp,a0                   ; save stack pointer

.cpu:
        lea     .cpu_done(pc),a1
        move.l  a1,4*4.w                ; illegal instruction handler
        ; VBR is 68010+
        moveq   #0,d1
        movec   d1,vbr
        bset    #AFB_68010,d0
        ; CACR is 68020+
        movec   cacr,d1
        bset    #AFB_68020,d0
        ; enable 68040/060 code cache
        move.l  #CACRF_EIC,d1
        movec   d1,cacr
        movec   cacr,d1
        ; bit 15 still set?
        tst.w   d1
        ; yes, it is 68040, 68060 or 68080
        bmi.s   .m040
        ; enable 68020/030 code cache and 68030 data cache
        move.l  #CACRF_EnableI|CACRF_EnableD,d1
        movec   d1,cacr
        movec   cacr,d1
        ; disable caches
        moveq   #0,d2
        movec   d2,cacr
        ; data cache bit still set?
        btst    #CACRB_EnableD,d1
        ; no, it is 68020
        beq     .cpu_done
        ; yes, it is 68030
        bset    #AFB_68030,d0
        bra     .cpu_done

.m040:  or.w    #AFF_68030|AFF_68040,d0
        moveq   #0,d1
        ; disable caches
        movec   d1,cacr
        ; data cache must be invalidated after reset
        cinva   dc
        ; set transparent translation registers,
        ; allow data caching only in 32-bit fast,
        ; code caching allowed everywhere
        movec   d1,itt1
        move.l  #$0000e040,d1
        movec   d1,dtt0
        move.l  #$00ffe000,d1
        movec   d1,dtt1
        movec   d1,itt0
        ; PCR is 68060 only
        movec   pcr,d1
        bset    #AFB_68060,d0
        cmp.w   #$0600,d1
        bcc.s   .rev6
        ; rev5 or older 68060 revision
        ; enable I14 and I15 errata workaround
        ; also enables FPU & supercalar dispatch
        moveq   #$21,d1
        movec   d1,pcr
        bra.s   .cpu_done
.rev6:  moveq   #$1,d1
        movec   d1,pcr

.cpu_done:
        move.l  a0,sp           ; remove exception stack frame

.fpu:
        lea     .fpu_done(pc),a1
        move.l  a1,11*4.w       ; f-line handler

        lea     -60(sp),sp
        move.l  sp,a1
        clr.w   (a1)
        fnop
        fsave   (a1)
        btst    #AFB_68040,d0
        beq.s   .m881
        bset    #AFB_FPU40,d0   ; no 68881/68882 emulation available
        bra.s   .fpu_done
.m881   bset    #AFB_68881,d0
        cmp.b   #$1f,1(a1)
        blt.s   .fpu_done
        bset    #AFB_68882,d0

.fpu_done:
        move.l  a0,sp           ; remove exception stack frame
        move.l  (sp)+,d2
        rts                     ; return from CpuDetect

; vim: ft=asm68k:ts=8:sw=8:et:
