#!/bin/bash

stop network-manager

#rmmod iwlwifi mac80211 cfg80211
#modprobe iwlwifi
modprobe iwlwifi  connector_log=0x1 
#modprobe iwlwifi  debug=0x40000
ifconfig wlan0 2>/dev/null 1>/dev/null
while [ $? -ne 0 ]
do
	        ifconfig wlan0 2>/dev/null 1>/dev/null
done
iw dev wlan0 interface add mon0 type monitor
iw mon0 set channel $1 $2
ifconfig mon0 up
ifconfig wlan0 down
echo 0x6100 |  tee ` find /sys -name monitor_tx_rate`


iwconfig wlan0 mode monitor 2>/dev/null 1>/dev/null
while [ $? -ne 0 ]
do
	iwconfig wlan0 mode monitor 2>/dev/null 1>/dev/null
done
iw wlan0 set channel $1 $2
ifconfig wlan0 up

echo "Done"
