#if defined(__H8300__)
	.h8300
#error Old H8/300 is not supported
#elif defined(__H8300H__)
#if defined(__NORMAL_MODE__)
	.h8300hn
#else
	.h8300h
#define LONG_POINTER 1
#endif
#elif defined(__H8300S__)
#if defined(__NORMAL_MODE__)
	.h8300sn
#else
	.h8300s
#define LONG_POINTER 1
#endif
#elif defined(__H8300SX__)
#if defined(__NORMAL_MODE__)
	.h8300sxn
#else
	.h8300sx
#define LONG_POINTER 1
#endif
#endif

	.macro mov.p src, dst
#ifdef LONG_POINTER
	mov.l	\src, e\dst
#else
	mov.w	\src, \dst
#endif
	.endm

	.equ	_abuf, _ram+0x0000
	.equ	_wbuf, _ram+0x0080
	.equ	_rbuf, _ram+0x0100
	.equ	_end, _ram+0x0200

#define FLMCR1 0xFFFFFF90
#define FLMCR2 0xFFFFFF91
#define EBR1   0xFFFFFF93
#define FENR   0xFFFFFF9B
#define SMR    0xFFFFFFA8
#define BRR    0xFFFFFFA9
#define SCR3   0xFFFFFFAA
#define TDR    0xFFFFFFAB
#define SSR    0xFFFFFFAC
#define RDR    0xFFFFFFAD
#define TCSRWD 0xFFFFFFC0
#define TCWD   0xFFFFFFC1
#define TMWD   0xFFFFFFC2
#define FLMCR1_SWE  #6, @FLMCR1:8
#define FLMCR1_ESU  #5, @FLMCR1:8
#define FLMCR1_PSU  #4, @FLMCR1:8
#define FLMCR1_EV   #3, @FLMCR1:8
#define FLMCR1_PV   #2, @FLMCR1:8
#define FLMCR1_ERS  #1, @FLMCR1:8
#define FLMCR1_PGM  #0, @FLMCR1:8
#define FLMCR2_FLER #7, @FLMCR2:8
#define FENR_FLASHE #7, @FENR:8
#define SCR3_TE     #5, @SCR3:8
#define SCR3_RE     #4, @SCR3:8
#define SCR3_CKE1   #1, @SCR3:8
#define SCR3_CKE0   #0, @SCR3:8
#define SSR_TDRE    #7, @SSR:8
#define SSR_RDRF    #6, @SSR:8
#define SSR_OER     #5, @SSR:8
#define SSR_FER     #4, @SSR:8
#define SSR_PER     #3, @SSR:8
#define SSR_TEND    #2, @SSR:8

#define loop(cond, op) loop_1 cond, op, 
	.macro loop_1 cond:req op:req args:vararg
.loop_macro_\@\():
	\op	\args
	\cond	.loop_macro_\@
	.endm

	.macro usleep_var reg
.usleep_macro_\@\():
	dec	#1, \reg
	bne	.usleep_macro_\@\():16
	.endm
	.macro usleep time, reg
	mov	#(\time*4), \reg
	usleep_var \reg
	.endm

	.section .text
	.align 2
	.global _start
	.org 0
_start:
	mov.p	#_end, r7
_main:
	bset	FENR_FLASHE
	mov.b	@SCR3:8, r1l
	mov.b	r1l, r1h
	or.b	#0x30, r1h
	and.b	#0xCF, r1l
	mov.b	r1h, @SCR3:8
	bsr	_usleep_400:16
	loop(bge, mov.b) @SSR:8, r2l
	mov.b	#'B', r2l
	mov.b	r2l, @TDR:8
	loop(beq, btst) SSR_RDRF
	mov.b	@RDR:8, r2l
	bsr	_usleep_400:16
	mov.b	r1l, @SCR3:8
	bsr	_usleep_400:16
	mov.b	r2l, @BRR:8
	bsr	_usleep_400:16
	mov.b	r1h, @SCR3:8
	bsr	_usleep_400:16
	mov.b	#'S', r2l
	mov.b	r2l, @TDR:8

	; er0: temporary
	; er1: wbuf
	; er2: end
	; er3: target
	loop(beq, btst) SSR_RDRF
	mov.b	@RDR:8, r2h
	loop(beq, btst) SSR_RDRF
	mov.b	@RDR:8, r2h
	loop(beq, btst) SSR_RDRF
	mov.b	@RDR:8, r2l
	mov.w	r2, e2
	loop(beq, btst) SSR_RDRF
	mov.b	@RDR:8, r2h
	sub.b	r2l, r2l

	sub.l	er3, er3
.main_loop:
	loop(beq, btst) SSR_TDRE
	mov.b	#'A', r0l
	mov.b	r0l, @TDR:8

	mov.p	#_wbuf, r1
	sub.b	r0h, r0h
.recv_loop:
	loop(beq, btst) SSR_RDRF
	mov.b	@RDR:8, r0l
	mov.b	r0l, @er1
	adds	#1, er1
	inc.b	r0h
	bpl	.recv_loop

	bsr	_flash:16
	mov.w	r0, r0
	bne	.error

	btst	FLMCR2_FLER
	bne	.error_1

	sub.l	er1, er1
	mov.b	#128, r1l
	add.l	er1, er3
	cmp.l	er2, er3
	blt	.main_loop
.success:
	mov.b	#'F', r0l
	bra	.finish
.error_1:
	mov.b	#'D', r0l
	bra	.finish
.error:
	mov.b	#'E', r0l
.finish:
	loop(beq, btst) SSR_TDRE
	mov.b	r0l, @TDR:8
	loop(bra, nop)

_usleep_400:
	usleep	400, r0
	rts

_flash: ; int flash(volatile void *target);
	; return 0 if success
	; target: er3
	; destroy: er0,er4,er5,er6

	; er3: target
	; e4: count
	; r4l, er5, er6: eepmov
	bset	FLMCR1_SWE

	mov.p	#_rbuf, r6
	mov.p	#_wbuf, r5
	mov.b	#128, r4l
	eepmov.b

	sub.l	er4, er4
.flash_loop_first:
	mov.l	er3, er6
	mov.p	#_rbuf, r5
	mov.b	#128, r4l
	eepmov.b

	mov.w	#30, r0
       	bsr	_pulse:16

	mov.l	er3, er0
	bsr	_verify:16
	push.w	r0

	mov.l	er3, er6
	mov.p	#_abuf, r5
	mov.b	#128, r4l
	eepmov.b

	mov.w	#10, r0
	bsr	_pulse:16

	pop.w	r0
	bne	.flash_success ; if verify ok, skip.

	inc.w	#1, e4
	cmp.w	#6, e4 ; continue if count <= 6
	ble	.flash_loop_first

.flash_loop_second:
	mov.l	er3, er6
	mov.p	#_rbuf, r5
	mov.b	#128, r4l
	eepmov.b

	mov.w	#200, r0
	bsr	_pulse:16

	mov.l	er3, er0
	bsr	_verify:16

	mov.w	r0, r0
	bne	.flash_success ; if verify ok, skip.

	inc.w	#1, e4
	cmp.w	#1000, e4; continue if count <= 1000
	ble	.flash_loop_second

.flash_error:
	mov.b	#1, r4l
	bra	.flash_finish
.flash_success:
	sub.l	er4, er4
.flash_finish:
	bclr	FLMCR1_SWE
	bsr	_usleep_400:16
	mov.l	er4, er0
	rts

_usleep_5:
	usleep	5, r0
	rts

_verify: ; bool verify(volatile void *target)
	; target: er0
	; destroy: er0,r4l,er5,er6
	; er0: counter
	; r4l: matched
	; er5: target
	; er6: temporary
	mov.b	#1, r4l
	mov.l	er0, er5

	bset	FLMCR1_PV
	bsr	_usleep_5:16

	sub.l	er0, er0 ; count = 0
.verify_loop:
	mov.b	#0xFF, r6h
	mov.b	r6h, @er5 ; *target = 0xFF
	usleep	2, e6
	mov.w	@er5+, r6 ; r6 = *target++
	mov.w	@(_rbuf,er0), e6 ; e6 = rbuf[count]
	or.w	r6, e6 ; e6 |= r6
	mov.w	e6, @(_abuf,er0) ; abuf[count] = e6
	mov.w	@(_wbuf,er0), e6
	cmp.w	e6, r6
	beq	.matched ; if wbuf[count] != r6 matched = 0
	sub.b	r4l, r4l
.matched:
	not.w	r6
	or.w	e6, r6 ; r6 = wbuf[count] | ~r6
	mov.w	r6, @(_rbuf,er0) ; rbuf[count] = r6
	add.b	#2, r0l
	bpl	.verify_loop

	bclr	FLMCR1_PV
	bsr	_usleep_5:16
	sub.l	er0, er0
	mov.b	r4l, r0l
	rts


_pulse: ; void pulse(unsigned usec);
	; usec: r0
	; destroy: er0
	; er0: temporary

	mov.w	r0, e0
	mov.w	#0xFF50, r0
	mov.b	r0h, @TMWD:8
	mov.b	r0l, @TCSRWD:8
	mov.w	#0xF056, r0
	mov.b	r0h, @TCWD:8
	mov.b	r0l, @TCSRWD:8

	bset	FLMCR1_PSU
	usleep	50, r0
	mov.w	#5, r0
	mulxu.w	e0, er0
	bset	FLMCR1_PGM
	usleep_var er0
	bclr	FLMCR1_PGM
	bsr	_usleep_5:16
	bclr	FLMCR1_PSU
	bsr	_usleep_5:16

	mov.b	#0x52, r0l
	mov.b	r0l, @TCSRWD:8
	rts

	.section .bss
	.lcomm _ram, 512
	;.lcomm _abuf, 128
	;.lcomm _wbuf, 128
	;.lcomm _rbuf, 128
