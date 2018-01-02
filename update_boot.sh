#!/bin/sh

petalinux-package --boot --force \
--fsbl images/linux/zynq_fsbl.elf \
--fpga images/linux/top.bit \
--u-boot images/linux/u-boot.elf \
-o images/linux/BOOT.BIN
