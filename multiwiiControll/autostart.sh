#!/bin/bash


sudo hostapd -B /etc/hostapd/hostapd.conf
sudo ifconfig wlan0 192.168.10.1
sudo service isc-dhcp-server restart
sudo python main.py &
