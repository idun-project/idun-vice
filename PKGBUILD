pkgname=idun-vice
pkgver=3.9
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
source=("$pkgname-$pkgver-$pkgrel.tar.gz")
md5sums=('c9c366a43d5f2b56c6f05b28eca13d98')

build() {
  cd vice/idun && make clean && make
  cd ..
  ./configure \
    --enable-sdl2ui \
    --without-pulse \
    --without-oss \
    --with-vorbis \
    --disable-pdf-docs \
    --disable-html-docs \
    --libdir=/usr/lib \
    --prefix=/usr
  make
  strip src/x*
}

package() {
  set USER=idun
  set HOME=/home/${USER}
  install -d -o ${USER} -g ${USER} "${pkgdir}"${HOME}/idun-vice/resc
  install -d -o ${USER} -g ${USER} "${pkgdir}"${HOME}/.config/vice
  install -m644 "${srcdir}"/vice/idun/resc/emu.rom "${pkgdir}"${HOME}/idun-vice/resc
  install -m644 "${srcdir}"/vice/idun/resc/emu64.rom "${pkgdir}"${HOME}/idun-vice/resc
  install -m644 "${srcdir}"/vice/idun/sdl-vicerc "${pkgdir}"${HOME}/.config/vice
  install -m755 "${srcdir}"/vice/idun/emu.sh "${pkgdir}"${HOME}/idun-vice
  install -m755 "${srcdir}"/vice/idun/emu64.sh "${pkgdir}"${HOME}/idun-vice
  cd "${srcdir}"/vice
  make DESTDIR="$pkgdir" install
}
