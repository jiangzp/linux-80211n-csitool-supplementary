#!/bin/sh

#This script will go to the csitool source dir, make the binaries and install them.

cd ../../linux-80211n-csitool	
cp /boot/config-'uname -r' .config
make oldconfig
make -j3 bzImage modules
sudo make install modules_install INSTALL_MOD_STRIP=1
sudo mkinitramfs -o /boot/initrd.img-`cat include/config/kernel.release` \
	`cat include/config/kernel.release`
sudo update-grub
sudo make headers_install	
sudo mkdir /usr/src/linux-headers-`cat include/config/kernel.release`
sudo cp -rf usr/include /usr/src/linux-headers-`cat include/config/kernel.release`/include
