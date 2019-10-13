.inc	"user.h"

.org	USER_CODE
	jp diceMain
.equ	DICE_RAMSTART	USER_RAMSTART
.inc	"dice/main.asm"