; Part of kernel because ideally we have some sort of entropy. For now,
; just having it be user-provided for prototypical purposes.
; This is the HiSoft C compiler output for a C Implementation written and compiled
; by John Metcalf of 
; http://www.retroprogramming.com/2017/07/xorshift-pseudorandom-numbers-in-z80.html
; (Slightly altered for CollapseOS)
;
; 16-bit xorshift pseudorandom number generator
; 
; in: hl = seed
; out: hl = pseudorandom number (generally fed into the next call)
; corrupts   a

rnd:
  ld a,h
  rra
  ld a,l
  rra
  xor h
  ld h,a
  ld a,l
  rra
  ld a,h
  rra
  xor l
  ld l,a
  xor h
  ld h,a

  ret