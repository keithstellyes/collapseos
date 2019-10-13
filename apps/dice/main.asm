; dice - A simple RNG application
; Generates a random number in the range of 0000-FFFF
; Perhaps would be handy to make more user-friendly in the-future 
; (and to use entropy). Right now, more proof-of-concept of RNG functionality
; than anything.

.equ SEED DICE_RAMSTART

diceMain:
	ld	hl, sInputSeed
	call	printstr
	call	stdioReadLine
	call	printcrlf
	xor	a ; ensure C is not set
	call	parseHexPair
	jp	c, unknownSeedInput
	ld	(SEED), a
	call	parseHexPair
	jp	c, unknownSeedInput
	ld (SEED+1), a
	ld hl, (SEED)
.diceMainLoop:
	call	stdioReadLine
	ld	a, (hl)
	cp	'q'
	jp z, diceExit
	ld	hl, (SEED)
	call	rnd
	ld	(SEED), hl
	call	printHexPair
	call	printcrlf
	jp .diceMainLoop

unknownSeedInput:
	ld a, '?'
	call	stdioPutC
	call	printcrlf
	jp diceMain

diceExit:
	call	printcrlf
	xor a
	ret

sInputSeed:
	.db	"Please input seed in hex (0000-FFFF):", 0