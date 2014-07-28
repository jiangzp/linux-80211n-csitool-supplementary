#!/bin/bash

sudo umount ./ramdisk
sudo mount -t tmpfs -o size=1024M,mode=777 tmpfs ./ramdisk
