pkgname=idun-vice
pkgver=3.6
pkgrel=1
pkgdesc="Patched VICE emulator for Idun"
arch=("x86_64" "armv7h")
url="https://github.com/idun-project/idun-vice"
license=(GPLv2)
depends=(alsa-lib giflib libjpeg-turbo libpng sdl2 sdl2_image)
makedepends=(dos2unix libpcap libxaw texlive-bin texlive-core xa xorg-bdftopcf)
provides=(idun-vice)
conflicts=(vice)
options=(emptydirs)
source=("$pkgname-$pkgver.tar.gz")
md5sums=('7b50f35e24f427c8880d4933b1dc662d')

build() {
  cd vice/idun && make clean && make
  cd ..
  ./configure \
    --enable-sdl2ui \
    --enable-gtk3ui \
    --without-oss \
    --disable-pdf-docs \
    --disable-ffmpeg \
    --with-fastsid \
    --libdir=/usr/lib \
    --prefix=/usr
  make -j4
  strip src/x*
}

package() {
  install -d -o idun -g idun "${pkgdir}"/home/idun/idun-vice/resc
  install -d -o idun -g idun "${pkgdir}"/home/idun/.config/vice
  install -m644 "${srcdir}"/vice/idun/resc/emu.rom "${pkgdir}"/home/idun/idun-vice/resc
  install -m644 "${srcdir}"/vice/idun/resc/emu64.rom "${pkgdir}"/home/idun/idun-vice/resc
  install -m644 "${srcdir}"/vice/idun/sdl-vicerc "${pkgdir}"/home/idun/.config/vice
  install -m755 "${srcdir}"/vice/idun/emu.sh "${pkgdir}"/home/idun/idun-vice
  install -m755 "${srcdir}"/vice/idun/emu64.sh "${pkgdir}"/home/idun/idun-vice
  cd "${srcdir}"/vice
  make DESTDIR="$pkgdir" install
}
