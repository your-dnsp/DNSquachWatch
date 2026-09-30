#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 /dev/cu.usbserial-XXX"
  echo "Linux ports are commonly /dev/ttyUSB0 or /dev/ttyACM0."
  exit 2
fi

cd "$(dirname "$0")"
for file in bootloader.bin partitions.bin boot_app0.bin firmware.bin; do
  if [ ! -f "$file" ]; then
    echo "Missing $file. Extract the complete release ZIP first."
    exit 1
  fi
done

python3 -m esptool --chip esp32 --port "$1" write_flash \
  --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000 bootloader.bin \
  0x8000 partitions.bin \
  0xe000 boot_app0.bin \
  0x10000 firmware.bin
