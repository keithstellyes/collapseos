; 8K of onboard RAM
.equ	RAMSTART	0xc000
; Memory register at the end of RAM. Must not overwrite
.equ	RAMEND		0xfdd0

	jp	init

.fill 0x66-$
	retn

.inc "err.h"
.inc "core.asm"
.inc "parse.asm"

.equ	PAD_RAMSTART	RAMSTART
.inc "sms/pad.asm"

.equ	VDP_RAMSTART	PAD_RAMEND
.inc "sms/vdp.asm"

.equ	STDIO_RAMSTART	VDP_RAMEND
.inc "stdio.asm"

.equ	SHELL_RAMSTART	STDIO_RAMEND
.equ	SHELL_EXTRA_CMD_COUNT 0
.inc "shell.asm"

init:
	di
	im	1

	ld	sp, RAMEND

	call	padInit
	call	vdpInit
	ld	hl, padUpdateSel
	ld	(VDP_CHRSELHOOK), hl

	ld	hl, padGetC
	ld	de, vdpPutC
	call	stdioInit
	call	shellInit
	ld	hl, vdpShellLoopHook
	ld	(SHELL_LOOPHOOK), hl
	jp	shellLoop

.fill 0x7ff0-$
.db "TMR SEGA", 0x00, 0x00, 0xfb, 0x68, 0x00, 0x00, 0x00, 0x4c
