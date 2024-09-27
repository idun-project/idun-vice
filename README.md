## Vice Emulator for Idun Cartridge

Developing for the idun cart can be done using Vice. This patched version of Vice connects to the Raspberry Pi on your Idun cartridge and lets you run an emulated environment either directly on the Raspberry Pi, or from a remote PC. Most of the features of the idun cart are available through the emulator, including developing apps that use Lua. What does not work is the `go64` and `load` commands from the idun shell.

This emulator can also work for development even if you lack the actual idun cartidge hardware. You can download the image for idun, burn it to an SD card, and use it to boot most versions of Raspberry Pi. You need a RPi with ARMv7, at least 512MB of RAM, and a 4GB SD card.

### Installation

To use the emulator on the Raspberry Pi connected with your idun cart, install the latest release from the idun package repository. If you are new to `pacman`, consider perusing the Arch Linux [pacman primer](https://wiki.archlinux.org/title/Pacman).

```
sudo pacman -Sy idun-vice
```

Connect a keyboard to your RPi, and start the emulator using the included launch script `idun-vice/emu.sh`. The emulator runs fullscreen using SDL2. Press F12 to access the Vice menu.

_Note_: `idun-vice/emu64.sh` is sort of ancillary. It exists mainly because the `go64` shell command doesn't work in the emulator. Instead, you can exit the x128 emulator (`emu.sh`), and start the x64sc emulator (`emu64.sh`), without upsetting the state of the cartridge.

### Building

This GitHub repo is fully buildable on a RPi.
```
git clone https://github.com/idun-project/idun-vice.git
cd idun-vice
./buildpkg.sh
```

If you want to build modified Vice binaries for other platforms, then start in directory `idun-vice/vice` and build things the normal way using `configure` and `make`, or follow the details for your platform in the Vice documentation.

### Using without idun-cartridge

- You will need to edit `~/.config/idunrc.toml` and comment out the setting for `io.serial` such as shown below. This will stop the RPi continually trying to re-connect with the cartridge, otherwise it's unusable!
```
[io]
#serial 	= "/dev/ttyAMA0"
```
- You will also need to make an addition to `/etc/systemd/system/idun.service`. In the section under `[Service]`, add an additional line
    + `Environment=IDUN_MODE_SWITCH=False`
    + This variable is used to toggle whether the Mode switch is seen as enabled when you don't have an actual idun-cart connected to the RPi.
- To connect to the RPi from a PC running the emulator, make sure `io.devsock` is set as shown below. Otherwise, only local connection is allowed!
```
[io]
devsock = "0.0.0.0:25232"	# allow remote emulator connection
```
