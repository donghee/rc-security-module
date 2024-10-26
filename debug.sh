#!/bin/sh

# gdb server
#pyocd gdbserver --persist -t stm32f479iihx
# for multiple targets
#pyocd gdbserver --persist -t stm32f479iihx -u 50FF6A064889495145330387 # TX
#pyocd gdbserver --persist -t stm32f479iihx -u 16004A002933353739303541 -p 3334 -T 4445 # RX

case "$1" in
  "TX")
    gdb-multiarch -x .pioinit-tx
    ;;
	"RX")
    gdb-multiarch -x .pioinit-rx
    ;;
	 *)
     echo "TX or RX"
     exit 1
     ;;
esac
