#!/bin/sh

start ()
{
	echo "Starting SIL9293."
	# Minimal initialization steps required for SIL9293 HDMI receivelr
	# software reset
	i2cset -y 0 0x32  0x05 0xd1
	# release reset
	i2cset -y 0 0x32  0x05 0xd0
	# Rx Control
	i2cset -y 0 0x68  0x6c 0x3f
	# termination
	i2cset -y 0 0x68  0x70 0xC0
	# HDMI Capability - set to DVI
	i2cset -y 0 0x32  0x2e 0x00
	# Hot plug
	i2cset -y 0 0x32  0x0b 0x01
	# Enable power
	i2cset -y 0 0x32  0x08 0x05
}

stop ()
{
	# Disable power
	i2cset -y 0 0x32  0x08 0x04
	# Hot plug
	i2cset -y 0 0x32  0x0b 0x00
	echo "SIL9293 stopped."
}

restart ()
{
	stop
	start
}

case "$1" in
	start)
		start; ;;
	stop)
		stop; ;;
	restart)
		restart; ;;
	*)
		echo "Usage: $0 {start|stop|restart}"
		exit 1
esac

exit $?

