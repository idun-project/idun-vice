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
  install -m644 "${srcdir}"/vice/idun/resc/emu.rom "${pkgdir}"/usr/share/idun/rom/emu.rom
  install -m644 "${srcdir}"/vice/idun/resc/emu64.rom "${pkgdir}"/usr/share/idun/rom/emu64.rom
  install -m755 "${srcdir}"/vice/idun/vice.sh "${pkgdir}"/usr/bin/vice
}
sha512sums="
b722193ab158ae53435561addab937a475195036e2d50dfd431ff99b2e5dccc14f161267bb8e847a0dfdec6c658f9cad81c96d01c2a86261c7c67e4dcb03f835  idun-vice-3.9.tar.gz
"
