# Maintainer: Brian Holdsworth <brian@focus42llc.com>
pkgname=idun-vice
pkgver=3.9
pkgrel=0
pkgdesc="Patched VICE emulator for Idun"
arch="aarch64"
url="https://github.com/idun-project/idun-vice"
license="GPLv2"
depends="alsa-lib giflib libjpeg-turbo libpng sdl3 sdl2-compat"
makedepends="sdl3-dev sdl2-compat-dev pkgconf
flex bison dos2unix curl-dev
libvorbis-dev" #libpcap xa xorg-bdftopcf
options="!check"
source=("$pkgname-$pkgver.tar.gz")
buildir="$srcdir"

build() {
  cd "${srcdir}"/vice/idun && make clean && make
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
  make -j 4
  strip src/x*
}

package() {
  cd "${builddir}"/vice
  make DESTDIR="$pkgdir" install
  # set USER=idun
  # set HOME=/home/${USER}
  # install -d -o ${USER} -g ${USER} "${pkgdir}"${HOME}/idun-vice/resc
  # install -d -o ${USER} -g ${USER} "${pkgdir}"${HOME}/.config/vice
  # install -m644 "${srcdir}"/vice/idun/resc/emu.rom "${pkgdir}"${HOME}/idun-vice/resc
  # install -m644 "${srcdir}"/vice/idun/resc/emu64.rom "${pkgdir}"${HOME}/idun-vice/resc
  # install -m644 "${srcdir}"/vice/idun/sdl-vicerc "${pkgdir}"${HOME}/.config/vice
  # install -m755 "${srcdir}"/vice/idun/emu.sh "${pkgdir}"${HOME}/idun-vice
  # install -m755 "${srcdir}"/vice/idun/emu64.sh "${pkgdir}"${HOME}/idun-vice
}
