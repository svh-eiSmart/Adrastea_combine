#!/bin/sh

if [[ -n "${ComSpec}" ]]; then
	CMD="${ComSpec} /c"
fi

${CMD} "$(dirname "$0")/genver.py" $1
