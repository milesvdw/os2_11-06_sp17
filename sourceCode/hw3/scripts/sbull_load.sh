#!/bin/sh

function make_minors {
    let part=1
    while (($part < $minors)); do
	let minor=$part+$2
	mknod $1$part b $major $minor
	let part=$part+1
    done
}


# FIXME: This isn't handling minors (partitions) at all.
module="sbull"
device="sbull"
mode="664"
chardevice="sbullr"
minors=16

# Group: since distributions do it differently, look for wheel or use staff
if grep '^staff:' /etc/group > /dev/null; then
    group="staff"
else
    group="wheel"
fi

# invoke insmod with all arguments we got
# and use a pathname, as newer modutils don't look in . by default
/sbin/insmod -f ./$module.ko $* || exit 1

major=`cat /proc/devices | awk "\\$2==\"$module\" {print \\$1}"`

# Remove stale nodes and replace them, then give gid and perms

rm -f /dev/${device}[a-d]* /dev/${device}

mknod /dev/${device}a b $major 0
make_minors /dev/${device}a 0
mknod /dev/${device}b b $major 16
make_minors /dev/${device}b 16
mknod /dev/${device}c b $major 32
make_minors /dev/${device}c 32
mknod /dev/${device}d b $major 48
make_minors /dev/${device}d 48
ln -sf ${device}a /dev/${device}
chgrp $group /dev/${device}[a-d]*
chmod $mode /dev/${device}[a-d]*