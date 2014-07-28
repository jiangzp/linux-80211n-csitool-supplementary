#!/bin/bash

sudo airmon-ng stop mon0 2>/dev/null 1>/dev/null
sudo ifconfig wlan0 down
sudo iwconfig wlan0 mode monitor
sudo ifconfig wlan0 up

sudo iw wlan0 set channel $1 $2
sudo airmon-ng start wlan0 2>/dev/null 1>/dev/null
sudo iw mon0 set channel $1 $2
sudo echo 0x$3 | sudo tee `sudo find /sys -name monitor_tx_rate`
echo "Done"
