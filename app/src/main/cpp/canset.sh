#!/system/bin/sh

setprop net.can.change no

new_baudrate=`getprop net.can.baudrate`

ifconfig can0 down

/system/bin/canset $new_baudrate
