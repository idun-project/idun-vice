# Maintainer: Brian Holdsworth <brian@focus42llc.com>
pkgname=idun-vice
pkgver=3.9
pkgrel=1
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
  install -m755 -d "${pkgdir}"/usr/share/idun/rom
  install -m644 "${builddir}"/vice/idun/resc/emu.rom "${pkgdir}"/usr/share/idun/rom/emu.rom
  install -m644 "${builddir}"/vice/idun/resc/emu64.rom "${pkgdir}"/usr/share/idun/rom/emu64.rom
  install -m755 "${builddir}"/vice/idun/vice.sh "${pkgdir}"/usr/bin/vice
}
sha512sums="
89b49db4fe2f3eedcd44bd899f1dfbe96a13298222ecc0ccd7d4ef8192c96ee5f55bd056d308feb0a329f1a3d578f3aa32b6f515c507f33aae3257334b898762  idun-vice-3.9.tar.gz
"
