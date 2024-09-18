#!/bin/sh

# gdb server
#pyocd gdbserver --persist -t stm32f479iihx

gdb-multiarch -x .pioinit
