## Vice Emulator for Idun Cartridge

Emulation of most of the Idun cart's functionality can be done with this modified version of Vice. This emulator can be run locally on the idun-cartridge, or remotely on another computer that can connect to the idun-cartridge across the network. When you run it, you have to pass an `idunhost` parameter, which is the network address/port where the emulator will find the cartridge. For the default setup, `idunhost` will need to be `idunpi.local:25232`.

 Most of the features of the idun cart are available through the emulator, including developing apps that use Lua. This makes the emulator very handy for developers working on Idun tools & applications. You can run it on your desktop, use the monitor and other debugging aids the emulator offers, then immediately step over to your Commodore and run the exact same binary there. No shuttling files around required, and no need to setup a "development environment" since you can build your software right on the cartridge itself!

This emulator can also work for development even if you lack the actual idun cartidge hardware. You can download the image for idun, burn it to an SD card, and use it to boot any current model Raspberry Pi.

### Installation

To use the emulator on the Raspberry Pi connected with your idun cart, install the latest release from the idun package repository.

```
sudo apk add idun-vice
```

Connect a keyboard to your RPi, and start the emulator using the included launch script `vice`. The emulator runs fullscreen using SDL3. Press F12 to access the Vice menu. The `vice` command is a sophisticated launcher. Run `vice -h` for a brief overview. The most important attributes of the launcher you need to know:

1. `vice` respects the mode switch position of the cartridge, and starts the emulator program that matches the mode (x128 or x64sc).
2. You can switch to the OTHER mode by just running `vice -o`.
3. You can disable integration with Idun by adding the `-d` switch.
4. You can run some other Vice program using the `-m` switch (e.g. `vice -m xpet`).
5. You can pass any additional emulator arguments for Vice by just appending them to the command.

### Building

In the likely event that you want to use idun-vice on a Windows desktop, you can just download the latest insaller release from the "Releases" section of the Github repository.

This GitHub repo is buildable on a RasPi, and most *nix systems. You will have to install all the generic autoconf and `C` language tooling first.
```
git clone https://github.com/idun-project/idun-vice.git
cd idun-vice/vice && ./autogen.sh
{Copy the ./configure ... from idun-vice/APKBUILD}
make
make install
```

### Enabling remote emulator

To connect to the cartridge from a PC running the emulator, you must edit `~/.config/idunrc.toml` and make sure `io.socket` is set as shown below. Otherwise, only local connection is allowed!
```
[io]
socket = "0.0.0.0:25232"	# allow remote emulator connection
```

### Using without idun-cartridge

When there is no cartridge, but just a "naked" Raspberry Pi, then the system has no way to toggle the Mode switch. The default will be assumed Mode "OFF", which normally means to boot on a C128. There are a couple of options if what you really want to boot is an emulated C64.

1. You can edit your idunrc.toml file, and change the options under `[start]`. If you set it up like a C64, then the default Mode "OFF" will give you a C64 emulated environment.
2. You can fake the Mode switch to "ON" with a simple command run from a Linux prompt on the RasPi:
`echo "1" | sudo tee /run/idun/modesw`
