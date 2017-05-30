#!/bin/tcsh

echo Entering correct directory
sleep 1
(cd /scratch/spring2017/11-06/linux-yocto-3.14)
echo Sourcing the enviroment files for QEMOU
sleep 1
(source /scratch/opt/environment-setup-i586-poky-linux.csh)
echo Launching VM with network, no GDB, and virtio
sleep 1
(qemu-system-i386 -nographic -kernel arch/i386/boot/bzImage -drive file=core-image-lsb-sdk-qemux86.ext3,if=virtio -enable-kvm -usb -localtime --no-reboot --append "root=/dev/vda rw console=ttyS0 debug")
