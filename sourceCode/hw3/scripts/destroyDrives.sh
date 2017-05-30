#!/bin/sh

echo Unmounting drives
sleep 1

(umount /dev/sbulla)
(umount /dev/sbullb)
(umount /dev/sbullc)
(umount /dev/sbulld)

echo Unloading Sbull module
(./sbull_unload.sh)

echo complete!
