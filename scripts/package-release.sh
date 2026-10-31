#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
version=${1:-dev}
package="ixfilm-os-${version}"
stage="$root/dist/$package"

stocks="
acros.fs
astia.fs
bleachbypass.fs
classicchrome.fs
classicneg.fs
eterna.fs
gold.fs
nostalgicneg.fs
portra.fs
provia.fs
realaace.fs
soup800.fs
velvia.fs
trix400.fs
hp5plus.fs
fp4plus.fs
tmax100.fs
delta3200.fs
"

need_file() {
    if [ ! -f "$1" ]; then
        echo "Missing required file: $1" >&2
        exit 1
    fi
}

need_file "$root/chdk-trunk/bin/DISKBOOT.BIN"
need_file "$root/chdk-trunk/CHDK/MODULES/ixfilm.flt"
need_file "$root/chdk-trunk/CHDK/MODULES/mpopup.flt"
need_file "$root/chdk-trunk/COPYING"

for stock in $stocks; do
    need_file "$root/soup-engine/src/stocks/$stock"
done

if [ -e "$stage" ]; then
    echo "Refusing to overwrite existing release directory: $stage" >&2
    exit 1
fi

mkdir -p "$stage/CHDK/FILM" "$stage/CHDK/MODULES" "$stage/CHDK/DATA" "$stage/CHDK/LOGS"
cp "$root/chdk-trunk/bin/DISKBOOT.BIN" "$stage/"
cp "$root/chdk-trunk/CHDK/MODULES/ixfilm.flt" "$stage/CHDK/MODULES/"
cp "$root/chdk-trunk/CHDK/MODULES/mpopup.flt" "$stage/CHDK/MODULES/"
cp "$root/chdk-trunk/COPYING" "$stage/COPYING"

for stock in $stocks; do
    cp "$root/soup-engine/src/stocks/$stock" "$stage/CHDK/FILM/"
done

cp "$root/README.md" "$stage/README.md"
cp "$root/CREDITS.md" "$stage/CREDITS.md"
cp -R "$root/docs" "$stage/docs"
sed -i.bak 's#](chdk-trunk/COPYING)#](COPYING)#' "$stage/README.md"
rm "$stage/README.md.bak"

(
    cd "$stage"
    shasum -a 256 DISKBOOT.BIN CHDK/MODULES/ixfilm.flt CHDK/MODULES/mpopup.flt > SHA256SUMS
)

echo "Created $stage"
echo "PS.FI2 is not included. Prepare the card with the official matching CHDK package first."

