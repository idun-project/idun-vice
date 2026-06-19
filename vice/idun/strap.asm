!source "bootconf.asm"

;** kernal entry points
kernelRAMTAS    = $ff87
kernelRESTOR    = $ff8a
kernelIOINIT    = $ff84
kernelCINT      = $ff81
kernelChrout    = $ffd2
kernelSetlfs    = $ffba
kernelSetnam    = $ffbd

;** kernal vars
kStatus     = $90 ; I/O status var
kFileaddr   = $ac
kCurraddr   = $ae
kFnlen      = $b7
kSecaddr    = $b9
kFnaddr     = $bb ; address of the filename
kSaveaddr   = $c1

!if useC128 {
    bkRam0 = $3f
    bkExtrom = %00001010
    bkSelect = $ff00

    kUseaddr    = $c3
    kFilebank   = $c6
    kFnbank     = $c7 ; bank of the filename stored probably 1

    kernelINDFET        = $ff74
    kernelSTA           = $f7bf
    kernelSetbnk        = $ff68
    kernalErrNotFound   = $f685
    kernalErrNotOpen    = $f682
    kernalErrMemory     = $f697
    hookCatalog = $1310
} else {
    bkExtrom = $37
    bkSelect = $01

    kernalErrNotFound   = $f703
    kernalErrNotOpen    = $f700
    kernalErrMemory     = $f700     ;???
    hookCatalog = $c00f
}

* = $8000
!if useC128 {
    jmp entryPoint  ;coldstart vector
    jmp warmstart   ;warmstart vector (not used)
    !byte $42       ;C128 cart id (started by PHEONIX)
    !pet  "cbm"
} else {
    !word entryPoint
    !word warmstart
    !pet "CBM80"
}

* = JUMP_ROM
    jmp Open
    jmp Loader
    jmp Saver
    jmp Close
    jmp hookCatalog
    
entryPoint = *
    lda #bkExtrom
    sta bkSelect
!if useC64 {
    jsr kernelIOINIT
    jsr kernelRAMTAS
    jsr kernelCINT
    jsr kernelRESTOR
}
warmstart = *
    ; wait for keyboard scan
    cli
    lda $a2
-   cmp $a2
    beq -
    sei
    ; this byte must be init'd to zero so the
    ; idun kernel loads the default shell.
    lda #0
    sta $b00
    ; idun boot device
    sta IdunDrive      ;@:
    ; stream booter cmd
    ldx #2
-   lda bootcmd,x
    jsr idunChOut
    dex
    bpl -
    ; select booter
    lda $91
    sta kSecaddr
    jsr idunChOut
    ; load booter and jump
    jsr LoaderCont
    jmp (kSaveaddr)
!if useC64 {
bootcmd !byte $f0,$7f,$20   ;CMD_STREAM_BOOTER_64
} else {
bootcmd !byte $ef,$7f,$20   ;CMD_STREAM_BOOTER_128
}
Loader = *
    lda #"P"
    jsr Open
    beq +
    jmp kernalErrNotFound
LoaderCont = *
+   jsr talk
    jsr idunChIn
    sta temp
    jsr idunChIn
    sta temp+1
    jsr setLoadaddr
    ;load file to memory @kCurraddr
    lda temp+1
-   beq +
    ldx #0
    jsr idunGetbuf
    jsr talk
    inc kCurraddr+1
    dec temp+1
    jmp -
+   ldx temp
    beq +
    jsr idunGetbuf
+   lda kCurraddr+0
    clc
    adc temp
    sta kCurraddr+0
    bcc +
    inc kCurraddr+1
; !if useC64 {
; +   cpy #>RAMR-1
;     bcc +
;     jsr kernelRESTOR
; }
+   jsr Close
    lda #$40
    sta kStatus
    ldx kCurraddr+0
    ldy kCurraddr+1
    clc
    rts
Saver = *
    lda #"+"
    jsr Open
    beq +
    jmp kernalErrNotOpen
+   jsr listen
    ;send size of file
    jsr setSavelen
    lda temp+0
    jsr idunChOut
    lda temp+1
    jsr idunChOut
    ;send address
    lda kSaveaddr+0
    jsr idunChOut
    lda kSaveaddr+1
    jsr idunChOut
    ;send file
-   lda kSaveaddr+1
    cmp kCurraddr+1
    bne +
    lda kSaveaddr+0
    cmp kCurraddr+0
    beq save_done
+   ldy #0
!if useC128 {
    ldx kFilebank
    lda #kSaveaddr
    jsr kernelINDFET
} else {
    lda (kSaveaddr),y
}
    sta idDataport
    inc kSaveaddr+0
    bne -
    inc kSaveaddr+1
    jmp -
    save_done = *
    ;UNLISTEN
    lda #$3f
    jsr idunChOut
    jsr Close
    clc
    rts

;-----------------------------------------------------------------------------
setSavelen = *
    lda kCurraddr+0
    sec
    sbc kSaveaddr+0
    sta temp+0
    lda kCurraddr+1
    sbc kSaveaddr+1
    sta temp+1
    ;add two bytes for addr header
    inc temp+0
    bne +
    inc temp+1
+   inc temp+0
    rts
setLoadaddr = *
    jsr idunChIn
    sta kCurraddr+0
    sta kSaveaddr+0
    jsr idunChIn
    sta kCurraddr+1
    sta kSaveaddr+1
    lda temp
    sec
    sbc #2
    sta temp
    bcs +
    dec temp+1
+   lda kSecaddr
    bne +
!if useC128 {
    lda kUseaddr+0
    ldx kUseaddr+1
} else {
    ;C64 start of BASIC
    lda #$01
    ldx #$08
}
    sta kCurraddr+0
    stx kCurraddr+1
+   rts

Open = *
    pha         ;push R/W flag
    lda IdunDrive
    clc
    ;send OPEN command
    adc #$20
    jsr idunChOut
    lda #$be    ;Lfn=30
    jsr idunChOut
    ;send filename
    ldy #0
-   cpy kFnlen
    beq +
!if useC128 {
    ldx kFnbank
    lda #kFnaddr
    jsr kernelINDFET
} else {
    lda (kFnaddr),y
}
    beq +
    sta idDataport
    iny
    jmp -
    ;send flag
+   pla
    cmp #"R"
    bne +
    ldx #$2c    ; ","
    stx idDataport
    nop
    jsr idunChOut
    ;end command
+   lda #0
    jsr idunChOut
    jsr idunFlush
    jsr unlisten
    bne +
    sta $6c
+   rts

talk = *
    lda #$01
    bit $6c
    beq +
    lda #$5f
    sta idDataport
    inc $6c
+   lda IdunDrive
    clc
    ;send TALK command
    adc #$40
    jsr idunChOut
    lda #$7e    ;Lfn=30
    jsr idunChOut
    inc $6c
    rts

listen = *
    lda IdunDrive
    clc
    adc #$20
    jsr idunChOut
    lda #$7e    ;Lfn=30
    jmp idunChOut

unlisten = *
    lda #$3F
    jsr idunChOut
    ; Get errno
    jmp idunChIn

Close = *
    lda #$01
    bit $6c
    beq +
    lda #$5f
    jsr idunChOut
    inc $6c
+   lda IdunDrive
    beq +
    clc
    adc #$20
    jsr idunChOut
    lda #$9e    ;Lfn=30
    jsr idunChOut
    jsr idunFlush
    jsr unlisten
+   rts

; ----------------------------------------------------------------------------
idDataport = $de00
idRxBufLen = $de01
idunChIn = *
    ; preserve X, Y
-   lda idRxBufLen
    beq -
    lda idDataport
    rts
idunChOut = *
    sta idDataport
    rts
idunFlush = *
-   lda idRxBufLen
    beq +
    lda idDataport
    jmp -
+   rts
; NOTE: This routine "borrows" some zp locations ($6a/$6b)
; that are used by BASIC for floating point calculations.
lengthBuf = $6a
recvAvail = $6b
idunGetbuf = *
    stx lengthBuf
    ldy #0
    ; wait for data available
--  lda idRxBufLen
    sta recvAvail
    beq --
    ; copy all available, up to lengthBuf
-   lda idDataport
!if useC128 {
    jsr kernelSTA
} else {
    sta (kCurraddr),y
}
    iny
    cpy lengthBuf
    beq +
    dec recvAvail
    beq --
    jmp -
+   rts
ROMEND = *

!if ROMEND-$8000 > 496 {
    !error "Max ROM size exceeded by ",ROMEND-$8000-ROMSIZE," bytes."
}
!if ROMEND-$8000 < ROMSIZE {
    * = $8000+ROMSIZE-1
    !byte 0
}
