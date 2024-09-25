#!/bin/sh

# gdb server
#pyocd gdbserver --persist -t stm32f479iihx

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
