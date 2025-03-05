!if computer-64 {
    useC128 = 1
    useC64  = 0
    LOADADDR = $1300
} else {
    useC128 = 0
    useC64  = 1
    LOADADDR = $c000
    RAMR     = $cf00
}
; These parameters may be altered by booter.
; Note: $9b/$9c are used by kernal tape loader
MyDevice    = $9b       ;(1)
IdunDrive   = $9c       ;(1)
; Temporary variable space (2 bytes)
temp        = $9d
;** ROM entry points
JUMP_ROM    = $800b
hookOpen    = <JUMP_ROM+0
hookLoad    = <JUMP_ROM+3
hookSave    = <JUMP_ROM+6
hookClose   = <JUMP_ROM+9
