# Maintainer: Brian Holdsworth <brian@focus42llc.com>
pkgname=idun-vice
pkgver=3.9
pkgrel=0
pkgdesc="Patched VICE emulator for Idun"
arch="aarch64"
url="https://github.com/idun-project/idun-vice"
license="GPLv2"
depends="alsa-lib giflib libjpeg-turbo libpng sdl3 sdl2-compat"
makedepends="sdl3-dev sdl2-compat-dev pkgconf flex bison dos2unix
curl-dev libvorbis-dev"
options="!check"
source="$pkgname-$pkgver.tar.gz"
builddir="$srcdir"

prepare() {
	cd "${srcdir}"/vice && source autogen.sh
}

build() {
  cd "${srcdir}"/vice/idun && make clean && make
  cd ..
  ./configure \
    --enable-sdl2ui \
    --without-pulse \
    --without-oss \
    --with-vorbis \
    --with-fastsid \
    --disable-ffmpeg \
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
sha512sums="
1a9d4563282386ac04f5e7844e52cc3f3c9cc8eb97154eaf8513b998aaaa07e3c57889f8fddaa80250da249f3d1f48e23d42caa9f0e20eb27a2dfc48a1bba417  idun-vice-3.9.tar.gz
"
