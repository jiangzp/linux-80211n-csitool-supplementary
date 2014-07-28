#!
sudo airmon-ng stop mon0
sudo ifconfig wlan0 down
sudo iwconfig wlan0 mode managed
sudo ifconfig wlan0 up
sudo start network-manager
