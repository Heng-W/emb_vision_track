#! /bin/bash

sleep 3s
#wpa_supplicant -i wlan0 -c /etc/wpa_supplicant/wpa_supplicant.conf -B
ifup wlan0
#route add default gw 192.168.2.1

service hostapd start
service udhcpd start

insmod /root/share/motor.ko
insmod /root/share/servo.ko
