pkgname=idun-vice
pkgver=3.6
pkgrel=2
pkgdesc="Patched VICE emulator for Idun"
arch=("x86_64" "armv7h")
url="https://github.com/idun-project/idun-vice"
license=(GPLv2)
depends=(alsa-lib giflib libjpeg-turbo libpng sdl2 sdl2_image)
makedepends=(pkgconf flex bison dos2unix libpcap xa xorg-bdftopcf)
provides=(idun-vice)
conflicts=(vice)
options=(emptydirs)
source=("$pkgname-$pkgver.tar.gz")
md5sums=('3d035d63064e9acec4fcc4e9f0b046c8')

build() {
  cd vice/idun && make clean && make
  cd ..
  ./configure \
    --enable-sdlui2 \
    --without-pulse \
    --without-oss \
    --with-vorbis \
    --disable-pdf-docs \
    --libdir=/usr/lib \
    --prefix=/usr
  make -j4
  strip src/x*
}

package() {
  install -d -o idun -g idun "${pkgdir}"${HOME}/idun-vice/resc
  install -d -o idun -g idun "${pkgdir}"${HOME}/.config/vice
  install -m644 "${srcdir}"/vice/idun/resc/emu.rom "${pkgdir}"${HOME}/idun-vice/resc
  install -m644 "${srcdir}"/vice/idun/resc/emu64.rom "${pkgdir}"${HOME}/idun-vice/resc
  install -m644 "${srcdir}"/vice/idun/sdl-vicerc "${pkgdir}"${HOME}/.config/vice
  install -m755 "${srcdir}"/vice/idun/emu.sh "${pkgdir}"${HOME}/idun-vice
  install -m755 "${srcdir}"/vice/idun/emu64.sh "${pkgdir}"${HOME}/idun-vice
  cd "${srcdir}"/vice
  make DESTDIR="$pkgdir" install
}
