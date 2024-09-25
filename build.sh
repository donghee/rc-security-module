#/home/donghee/.platformio/penv/bin/pio run -e genericSTM32F479IIHX --target clean && /home/donghee/.platformio/penv/bin/pio debug -e genericSTM32F479IIHX

case "$1" in
  "TX")
$HOME/.platformio/penv/bin/pio run -e TX --target clean && $HOME/.platformio/penv/bin/pio debug -e TX
    ;;
	"RX")
$HOME/.platformio/penv/bin/pio run -e RX --target clean && $HOME/.platformio/penv/bin/pio debug -e RX
    ;;
	 *)
     echo "TX or RX"
     exit 1
     ;;
esac
