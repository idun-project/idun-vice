## Vice Emulator for Idun Cartridge

Developing for the idun cart can be done using Vice. This patched version of Vice connects to the Raspberry Pi on your Idun cartridge and lets you run an emulated environment either directly on the Raspberry Pi, or from a remote PC. Most of the features of the idun cart are available through the emulator, including developing apps that use Lua. What does not work is the `go64` and `load` commands from the idun dos shell.

This emulator can also work for development even if you lack the actual idun cartidge hardware. You can download the image for idun, burn it to an SD card, and use it to boot most versions of Raspberry Pi. You need a RPi with ARMv7, at least 512MB of RAM, and a 4GB SD card.

### Installation

To use the emulator on the Raspberry Pi connected with your idun cart, install the latest release of Vice from this GitHub page onto your cartridge. If you are new to `pacman`, consider perusing the Arch Linux [pacman primer](https://wiki.archlinux.org/title/Pacman). You may need to sync with the remote repository using `pacman -Sy` before continuing.

```
sudo pacman -U idun-vice-3.6-1-armv7h.pkg.tar.zst
```

Connect a keyboard to your RPi, and start the emulator using the included launch script `idun-vice/emu.sh`. The emulator runs fullscreen using SDL2. Press F12 to access the Vice menu.

### Building

This GitHub repo is fully buildable on a RPi.
```
git clone https://github.com/idun-project/idun-vice.git
cd idun-vice
./buildpkg.sh
```

If you want to build modified Vice binaries for other platforms, then start in directory `vice` and build things the normal way using `configure` and `make`.

### Using without Idun Cartridge

- You will need to edit `~/.config/idunrc.toml` and comment out the setting for `io.serial` such as shown below. This will prevent the idun software from continually trying to connect with the cartridge, otherwise it's unusable!
```
[io]
#serial 	= "/dev/ttyAMA0"
```
- To connect to the RPi from a PC running the emulator, make sure `io.devsock` is set as shown below. Otherwise, only local connection is allowed!
```
[io]
devsock = "0.0.0.0:25232"	# allow remote emulator connection
```
