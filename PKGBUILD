pkgname=idun-vice
pkgver=3.8
pkgrel=1
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
md5sums=('1259daf3e42eb9a8c49c518a1bed1b33')

build() {
  cd vice/idun && make clean && make
  cd ..
  ./configure \
    --enable-sdl2ui \
    --without-pulse \
    --without-oss \
    --with-vorbis \
    --libdir=/usr/lib \
    --prefix=/usr
  make -j8
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
