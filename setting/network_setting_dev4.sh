#!/bin/sh

sudo ifconfig eth1 192.168.2.1 netmask 255.255.255.0
sudo route add -net 192.168.1.0 netmask 255.255.255.0 gw 192.168.2.2 dev eth1
