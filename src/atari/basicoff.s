        .export  basicoff_init
        .segment "BASICOFF"

; Hardware addresses
PORTB   = $D301      ; On XL/XE: memory control (BASIC at $A000-BFFF)
RAMTOP = $006A
RAMSIZ  = $02E4      ; cc65/OS top-of-usable memory (lo,hi)

; basicoff_init:
;  - Set BASICF so warmstarts don't re-enable BASIC.
;  - Must end with RTS (loader expects that for INITAD).

basicoff_init:
    lda #$c0       ; is RAMTOP already correct?
    cmp RAMTOP     ;
    beq ramok
        sta RAMTOP  ; Set RAMTOP to end of BASIC
        sta RAMSIZ  ; Set RAMSIZ also
        
        lda PORTB   ; Disable BASIC bit in PORTB for MMU
        ora #$02
        sta PORTB
        
        lda $a000   ; Check if BASIC ROM area is now writeable
        inc $a000
        cmp $a000
        beq ramnok  ; If not, perform error handling....
        
        lda #$01    ; Set BASICF for OS, so BASIC remains OFF after RESET
        sta $3f8
        
        ldx #2      ; Close "E:" before re-openining it again
        jsr editor
        ldx #0      ; Open "E:" to ensure screen is not at $9C00

editor:  lda $e401,x ; This prevents garbage when loading up to $bc000
        pha
        lda $e400,x
        pha

ramok:
    rts

ramnok:  inc 712     ; TODO: some sort of error handling.
        jmp ramnok
        

