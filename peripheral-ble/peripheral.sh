#!/bin/bash

[ -z "$FIRMWARE_DIR" ] && FIRMWARE_DIR="$(pwd)"

renode -e \
    "include @scripts/single-node/nrf52840.resc
emulation CreateServerSocketTerminal 1234 'externalUART'
connector Connect sysbus.uart0 externalUART
sysbus LoadELF @${FIRMWARE_DIR}/.pio/build/nrf52840_dk/firmware.elf
start" &>output_peripheral.log
