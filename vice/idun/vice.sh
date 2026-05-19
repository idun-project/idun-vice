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
            	idun_enabled=0
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
        [[ $opposite -eq 1 ]] && emulator="x128" && export IDUN_MODE_SWITCH=0
    else
        emulator="x128"
        [[ $opposite -eq 1 ]] && emulator="x64" && export IDUN_MODE_SWITCH=1
    fi
fi

# ==========================================
# Resource Generation (Consolidated)
# ==========================================

# Create temporary file for Vice resource configuration
temp_conf=$(mktemp /tmp/vice_res.XXXXXX)

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
            emu_args+=("-cartidun128" "/usr/share/idun/rom/emu.rom")
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
# Environment Configuration
# ==========================================

# Define potential socket locations
WAYLAND_SOCKET="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}/${WAYLAND_DISPLAY:-wayland-0}"

# Fallback check: look for any wayland-ish socket in common locations if the above fails
if [[ -S "$WAYLAND_SOCKET" ]] || [[ -S "/tmp/wayland-0" ]]; then
    export SDL_VIDEO_DRIVER=wayland
    echo "Display driver: Wayland socket detected"
else
    export SDL_VIDEO_DRIVER=kmsdrm
    echo "Display driver: No Wayland socket found, defaulting to KMS/DRM"
fi

# ==========================================
# Process Synchronization & Execution
# ==========================================

# Check whether idunkvm process is already running
kvm=1
if ! pgrep -x "idunkvm" > /dev/null; then
	kvm=0
fi

# Check if a monitor is actively sending an EDID signal
if ! grep -q "^connected$" /sys/class/drm/card*-*/status 2>/dev/null; then
    echo "WARNING: No active display detected on the DRM planes." >&2
    echo "--> Please verify your HDMI monitor is plugged in and POWERED ON! <--" >&2
    exit 1
fi

echo "Starting $emulator emulator..."
[[ $idun_enabled -eq 1 ]] && echo "Idun cartridge functionality: ENABLED" || echo "Idun cartridge functionality: DISABLED"
[[ $idun_enabled -eq 1 ]] && echo "To end emulation session, exit the emulator then press RESET button."

# 1. Print the whole command
echo -e "\n[Sanity Check] Launching with command:"
echo "$emulator -addconfig \"$temp_conf\" ${emu_args[*]}"

# 2. Launch backgrounded
nohup "$emulator" -addconfig "$temp_conf" "${emu_args[@]}" >/dev/null 2>&1 &

# 3. Trigger kvm switch if not already active and emulator will access Idun
if [[ $kvm -eq 0 && $idun_enabled -eq 1 ]]; then
	# Ensure the socket exists before trying to write to it
    if [[ -S "/tmp/idunmm-lua" ]]; then
        echo "sys.keystroke(171)" | socat - UNIX-CONNECT:/tmp/idunmm-lua
    fi
fi

sleep 0.5
exit 0
