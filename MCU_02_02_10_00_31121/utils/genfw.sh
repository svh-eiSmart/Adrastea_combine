#!/bin/sh

arm-none-eabi-objcopy -O binary "${1%bin}elf" "$1"

if [[ -n "${ComSpec}" ]]; then
	CMD="${ComSpec} /c"
fi

${CMD} "$(dirname "$0")/genfw.py" "$1"
