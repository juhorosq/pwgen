#!/bin/bash

exe="${1}"

demo_cmds=(
	"${exe} --version"
	"${exe} --help"
	"${exe}"
	"${exe} -l 16 -c 3"
	"for i in 1 2 3; do ${exe} -l 16 -c 1; done"
	"${exe} -l 16 -c 1 --random-seed=no_such_file"
	"for i in 1 2 3; do ${exe} -l 16 -c 1 --random-seed=no_such_file 2>/dev/null; done"
	"${exe} --symbols=num"
	"${exe} --count 10 --length=20 -Snum -- -+= .:"
	"${exe} -l 50 -c 10 -S ALPHA -S ALPHA -S alpha  # 2/3rds uppercase, 1/3rd lowercase"
	"${exe} -l 10 -c 10 ________x  # One x per word (on average)"
	"time ${exe} -l 11 -c 79000000 -r/dev/zero ' Hello' | grep 'Hello Hello'  # should take about 10 seconds"
)

set -e

if [ -t 1 ]; then # output is a terminal
	ncolors=$(tput colors)
	if [ -n "${ncolors}" ] && [ "${ncolors}" -ge 8 ]; then
		HL_START=$(tput bold)  # Start bold text
		HL_END=$(tput sgr0)    # Turn off all attributes
	fi
fi

for cmd in "${demo_cmds[@]}"; do
	echo -e "${HL_START}${cmd}${HL_END}"
	eval "${cmd}"
	echo
done
