#!/bin/sh

echo Loading sbull
sleep .2
(./sbull_load.sh)
sleep .2

echo Creating file system on SBULL device
sleep .2
(mkfs.ext2 /dev/sbulla)
sleep .2
(mkfs.ext2 /dev/sbullb)
sleep .2
(mkfs.ext2 /dev/sbullc)
sleep .2
(mkfs.ext2 /dev/sbulld)

echo Mounting ramdisks
sleep .2
(mount -t ext2 /dev/sbulla /mnt/sbullD1)
sleep .2
(mount -t ext2 /dev/sbullb /mnt/sbullD2)
sleep .2
(mount -t ext2 /dev/sbullc /mnt/sbullD3)
sleep .2
(mount -t ext2 /dev/sbulld /mnt/sbullD4)
sleep .2
echo Done...

(lsblk -fs)

