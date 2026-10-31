# Installing IXFilm OS

This guide is intentionally more careful than clever. Formatting the wrong disk is a memorable experience, but not in a useful way.

## Before you start

You need:

- a Canon IXUS 120 IS, PowerShot SD940 IS or IXY DIGITAL 220 IS;
- Canon firmware **1.00E**;
- an SD card and card reader;
- the IXFilm OS release archive;
- a backup of every photograph on the card.

The camera itself is not flashed. IXFilm starts from the SD card and only while that card is locked.

## Easy installation with the 4GB card image

Each GitHub release includes `ixfilm-os-VERSION-4gb-card.img.zip`. This is a cleaned, ready-to-boot copy of a real IXFilm card. It contains no photographs, logs or personal configuration.

This is the simplest installation method:

1. Download and extract the image archive from the release page.
2. Write the `.img` file to the whole SD card with a raw-image writer. Copying the file onto the card is not enough.
3. Eject the card cleanly.
4. Move its physical switch to **LOCK**.
5. Put it in the camera and start with the normal power button.

On Windows, use a trusted raw-image writer that supports `.img` files. On macOS or Linux, you can use a graphical image writer or `dd` if you are comfortable identifying disks from the command line.

The image contains a 2GB FAT16 partition and is intended for cards sold as 4GB or larger. Small differences between nominal 4GB cards are harmless because the image ends just after that 2GB partition. On a larger card, the remaining capacity is left unused. IXFilm and this camera are perfectly comfortable with 2GB of photo space.

Writing the image destroys every partition and file currently on the target card. Check the selected disk twice. The image is only for the Canon IXUS 120 IS, PowerShot SD940 IS or IXY DIGITAL 220 IS running firmware 1.00E.

The manual instructions below remain useful when you want a custom partition layout, want to use all of a larger card or prefer to prepare the card yourself.

## Check the firmware version

The release is built for firmware 1.00E only. Do not continue if your camera reports another version.

Canon hides the detailed version screen. The usual method for this model is:

1. Create an empty file named `ver.req` in the root of an unlocked SD card.
2. Put the card in the camera and start it with the `PLAY` button.
3. Hold `FUNC./SET` and press `DISP`.
4. Look for `Firmware Ver GM1.00E`.

Delete `ver.req` afterwards. It is not part of IXFilm.

## Choose the card layout

### Cards up to and including 4 GB

Use one MBR partition formatted as FAT16. On a 4 GB card, use 64 KB allocation units.

In the finished installation, the complete IXFilm release goes on this partition. `DISKBOOT.BIN` must be in the root.

### Cards larger than 4 GB

This camera cannot autoboot directly from a large FAT32 partition. Use:

1. a small FAT16 partition named `IXBOOT`;
2. a FAT32 partition named `IXFILM` using the remaining space.

In the finished installation, put `DISKBOOT.BIN` in the root of `IXBOOT` and put the `CHDK` folder on `IXFILM`. For the first release, also copy `DISKBOOT.BIN` to `IXFILM`; this makes recovery and card maintenance less confusing when the camera has swapped the visible partition.

Do not use exFAT or GPT. The partition table must be MBR, sometimes called Master Boot Record or DOS.

## Windows

### Format a card up to 4 GB

Open **Terminal (Administrator)** or **Command Prompt (Administrator)**, then run:

```text
diskpart
list disk
select disk N
detail disk
clean
convert mbr
create partition primary
format fs=fat unit=64k quick label=IXFILM
assign
exit
```

Replace `N` with the SD card's disk number. Check `detail disk` before `clean`. If its size does not match the card, stop.

### Partition a card larger than 4 GB

In an administrator terminal:

```text
diskpart
list disk
select disk N
detail disk
clean
convert mbr
create partition primary size=16
format fs=fat quick label=IXBOOT
assign
create partition primary
format fs=fat32 quick label=IXFILM
assign
exit
```

Windows normally shows both partitions on current versions.

## macOS

Find the whole SD card, not one of its partitions:

```sh
diskutil list external physical
```

The examples use `/dev/diskN`. Replace it with the identifier of the SD card and check its size before continuing.

### Format a card up to 4 GB

Create an MBR partition, then explicitly format it as FAT16:

```sh
diskutil partitionDisk /dev/diskN MBR MS-DOS IXFILM 100%
diskutil unmountVolume /dev/diskNs1
sudo newfs_msdos -F 16 -c 128 -v IXFILM /dev/rdiskNs1
diskutil mount /dev/diskNs1
```

`-c 128` means 128 sectors per cluster, or 64 KB with normal 512-byte sectors.

### Partition a card larger than 4 GB

```sh
diskutil partitionDisk /dev/diskN MBR MS-DOS IXBOOT 16M MS-DOS IXFILM R
diskutil unmountVolume /dev/diskNs1
sudo newfs_msdos -F 16 -v IXBOOT /dev/rdiskNs1
diskutil mount /dev/diskNs1
```

Disk Utility will normally format the large second partition as FAT32. Check it with `diskutil info /dev/diskNs2` before copying files.

## Linux

The commands below require `parted` and `mkfs.fat`, usually supplied by the `parted` and `dosfstools` packages.

Find the SD card:

```sh
lsblk -o NAME,SIZE,MODEL,TRAN,FSTYPE,MOUNTPOINTS
```

The examples use `/dev/sdX`. Replace it with the whole SD card. Unmount any mounted card partitions first.

### Format a card up to 4 GB

```sh
sudo parted -s /dev/sdX mklabel msdos
sudo parted -s /dev/sdX mkpart primary fat16 1MiB 100%
sudo mkfs.fat -F 16 -s 128 -n IXFILM /dev/sdX1
```

### Partition a card larger than 4 GB

```sh
sudo parted -s /dev/sdX mklabel msdos
sudo parted -s /dev/sdX mkpart primary fat16 1MiB 17MiB
sudo parted -s /dev/sdX mkpart primary fat32 17MiB 100%
sudo mkfs.fat -F 16 -n IXBOOT /dev/sdX1
sudo mkfs.fat -F 32 -n IXFILM /dev/sdX2
```

## Make the card bootable with CHDK

This step is the same on Windows, macOS and Linux. IXFilm relies on CHDK's official installer to create the boot marker. It does not include its own `PS.FI2`.

1. Leave the card's physical switch unlocked.
2. Follow the [official CHDK card-preparation instructions](https://chdk.fandom.com/wiki/Prepare_your_SD_card).
3. Download the CHDK build for the Canon IXUS 120 IS / SD940 IS with firmware **1.00E** from the [CHDK autobuild server](https://www.mighty-hoernsche.de/).
4. Copy the matching CHDK package to the card, including its `PS.FI2`. On a two-partition card, use the small FAT16 `IXBOOT` partition for this temporary preparation step.
5. Start the camera with the `PLAY` button.
6. Open Canon's menu and choose **Firm Update**.
7. In CHDK, choose **Make card bootable**.
8. Switch the camera off and put the card back in the computer.
9. Delete CHDK's `PS.FI2`, `DISKBOOT.BIN` and `CHDK` folder from the card. Do not format the card again, because formatting would remove the boot marker.
10. Continue with **Copy the IXFilm files** below.

Despite Canon calling this **Firm Update**, CHDK does not replace or flash Canon's firmware. It runs temporarily from the card and writes the boot marker to that card.

Only use a CHDK package that explicitly matches this camera and firmware 1.00E. Firmware-update files for other models or firmware versions are not interchangeable.

IXFilm does not redistribute CHDK's `PS.FI2` and does not extract, recreate or redistribute its non-public encoding material. The firmware-update file is only needed while preparing the card and should not remain in the finished IXFilm installation.

## Copy the IXFilm files

The release archive will contain this layout:

```text
DISKBOOT.BIN
CHDK/
  FILM/
    acros.fs
    ...
  MODULES/
    ixfilm.flt
    mpopup.flt
```

Copy folders as folders. Do not put the contents of `CHDK` directly in the card root.

For a single-partition card, everything goes on that partition. For a two-partition card, follow the placement described under **Cards larger than 4 GB**.

## First start

1. Eject the card cleanly.
2. Slide the card's physical switch to **LOCK**.
3. Put it in the camera.
4. Start the camera with the normal power button.
5. Wait for the message telling you to hold `DISP` for the film-simulation menu.

Canon cameras ignore the lock switch for saving photographs. The lock is used here as the autoboot signal.

If the camera starts normally without the IXFilm message, switch it off and check the filesystem, boot marker, physical lock switch and exact firmware version.

## One Canon setting worth changing

Set **Lens retract** to **0 sec.** in the normal Canon menu. IXFilm leaves this setting alone.

During conversion, press `PLAY`. Native playback mode turns the display to playback and retracts the lens immediately. Conversion continues in the background, so the camera can go into a pocket with the lens safely retracted.

## Removing IXFilm

Switch the camera off and unlock the SD card. The camera will start without IXFilm. To remove it permanently from that card, reformat the card or delete `DISKBOOT.BIN` and the IXFilm `CHDK` folder after saving your photographs.

