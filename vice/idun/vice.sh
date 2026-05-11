#!/bin/bash

# ==========================================
# Helper Functions
# ==========================================

show_help() {
    cat << EOF
Usage: $(basename "$0") [OPTIONS] [EMULATOR_ARGS...]

Custom launcher script for the Vice emulator with Idun cartridge support.

Options:
  -m <emulator>  Manually select the emulator to launch (e.g., xpet, xvic).
  -o             Opposite mode: reverse the default x64/x128 emulator selection.
  -d             Disable Idun cartridge functionality (Enabled by default).
  -h, --help     Show this help message and exit.

All other arguments will be passed directly to the emulator launch command.
EOF
}

# ==========================================
# Argument Parsing
# ==========================================

opposite=0
idun_enabled=1
machine=""
emu_args=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)
            show_help
            exit 0
            ;;
        -o)
            opposite=1
            shift
            ;;
        -d)
            idun_enabled=0
            shift
            ;;
        -m)
            if [[ -n "$2" ]]; then
                machine="$2"
                shift 2
            else
                echo "Error: -m requires an emulator argument (e.g., xpet)." >&2
                exit 1
            fi
            ;;
        *)
            # Pass-through arguments
            emu_args+=("$1")
            shift
            ;;
    esac
done

# ==========================================
# Determine Emulator Selection
# ==========================================

if [[ -n "$machine" ]]; then
    emulator="$machine"
else
    # Read modesw, default to 0 if missing
    modesw=0
    if [[ -f "/run/idun/modesw" ]]; then
        modesw=$(cat "/run/idun/modesw" | tr -d '[:space:]')
    fi

    # Determine default emulator based on modesw and opposite flag
    if [[ "$modesw" == "1" ]]; then
        emulator="x64"
        [[ $opposite -eq 1 ]] && emulator="x128"
    else
        emulator="x128"
        [[ $opposite -eq 1 ]] && emulator="x64"
    fi
fi

# ==========================================
# Resource Generation (Consolidated)
# ==========================================

# Create temporary file and ensure it is cleaned up on exit
temp_conf=$(mktemp /tmp/vice_res.XXXXXX)
trap 'rm -f "$temp_conf"' EXIT

resources=""

case "$emulator" in
    x64)
        resources="[C64SC]\nVICIIFilter=0\nVICIIGLFilter=0"
        if [[ $idun_enabled -eq 1 ]]; then
            resources+="\nIDUNIO=1\nIDUNHOST=127.0.0.1:25232"
            emu_args+=("-cartidun" "/usr/share/idun/rom/emu64.rom")
        fi
        ;;
    x128)
        resources="[C128]\nVICIIFilter=0\nVDCFilter=0\nVICIIGLFilter=0"
        if [[ $idun_enabled -eq 1 ]]; then
            resources+="\nIDUNIO=1\nIDUNHOST=127.0.0.1:25232"
            emu_args+=("-cartidun" "/usr/share/idun/rom/emu.rom")
        fi
        ;;
    xvic)
        resources="[VIC20]\nVICFilter=0\nVICIIGLFilter=0"
        ;;
    xplus4)
        resources="[PLUS4]\nTEDFilter=0\nVICIIGLFilter=0"
        ;;
    xpet)
        resources="[PET]\nCRTCFilter=0\nVICIIGLFilter=0"
        ;;
    xcbm2|xcbm5x0)
        resources="[CBM-II]\nCrtcFilter=0\nVICIIGLFilter=0"
        ;;
    x64dtv)
        resources="[C64DTV]\nDTVFilter=0\nVICIIGLFilter=0"
        ;;
esac

# Write consolidated resources to the temp file
echo -e "$resources" > "$temp_conf"

# ==========================================
# Process Synchronization & Execution
# ==========================================

# Wait for the idunkvm process to exist
if ! pgrep -x "idunkvm" > /dev/null; then
    echo -n "Switch keyboard and mouse control now <Cmd+k>...waiting"
    while ! pgrep -x "idunkvm" > /dev/null; do
        sleep 0.5
    done
    echo "" # New line once the process is found
fi

echo "Starting $emulator emulator..."
[[ $idun_enabled -eq 1 ]] && echo "Idun cartridge functionality: ENABLED" || echo "Idun cartridge functionality: DISABLED"
[[ $idun_enabled -eq 1 ]] && echo "To end emulation session, exit the emulator then press RESET button."

# 1. Print the whole command
echo -e "\n[Sanity Check] Launching with command:"
echo "$emulator -addconfig \"$temp_conf\" ${emu_args[*]}"

# 2. Launch backgrounded & 3. Exit
"$emulator" -addconfig "$temp_conf" "${emu_args[@]}" >/dev/null 2>&1 &
disown

sleep 1
exit 0
