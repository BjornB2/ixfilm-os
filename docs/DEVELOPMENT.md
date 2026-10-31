# Building IXFilm OS

This is the developer note, not the end-user installation guide. End users should install a tested release archive.

## Source layout

- `chdk-trunk/` contains the modified CHDK tree and the IXUS 120 IS camera integration.
- `soup-engine/` contains the image-processing engine and film recipes.
- `scripts/package-release.sh` creates the minimal card archive.

IXFilm currently builds only for `ixus120_sd940` firmware `100e`.

## Toolchain

The tested compiler is xPack GNU Arm Embedded GCC 11.3.1. A local toolchain directory is deliberately ignored by Git. Do not commit compiler binaries.

The CHDK build expects the usual `arm-none-eabi-` programs on `PATH`. From `chdk-trunk`:

```sh
make PLATFORM=ixus120_sd940 PLATFORMSUB=100e fir
```

The expected release outputs are:

```text
chdk-trunk/bin/DISKBOOT.BIN
chdk-trunk/CHDK/MODULES/ixfilm.flt
chdk-trunk/CHDK/MODULES/mpopup.flt
chdk-trunk/CHDK/MODULES/fselect.flt
```

Run the packager from the repository root:

```sh
scripts/package-release.sh 0.1.0-rc1
```

It creates `dist/ixfilm-os-0.1.0-rc1/` and records SHA-256 hashes for the four executable files. It refuses to overwrite an existing package.

## Windows bootstrap

Windows users prepare the card with the official CHDK package for the IXUS 120 IS / SD940 IS firmware 1.00E. They run its firmware-update procedure once and choose **Make card bootable**. After switching the camera off, they delete CHDK's card files and copy the IXFilm release onto the card. The boot marker is stored in the FAT16 volume boot sector and survives deleting those files.

IXFilm does not ship a `PS.FI2`. Generating one requires FI2 encoding material that CHDK deliberately keeps outside its public source tree. This project will not extract, reverse-engineer or redistribute that material.

The official CHDK preparation step is therefore part of the documented end-user installation, not merely a development fallback. The guide must credit CHDK and link to its own instructions and matching camera build.

## Licence obligations

Both the CHDK-derived runtime and Soup Engine are GPL code. A binary release must be accompanied by the complete corresponding source, build instructions, the licence and prominent attribution. Shipping only the card zip is not sufficient.

Before the first public commit, the nested Soup Engine checkout must be converted into normal tracked source or a clearly pinned submodule plus a tracked IXFilm patch set. A developer should be able to reproduce the binaries without relying on files outside the repository.

