#!/bin/sh
# Wrapper script for phosphor-gpio-presence.
# Allow for optional delay parameter for presence active and binding device driver.
# ExecStart=/usr/bin/phosphor-gpio-presence.sh ${DEVPATH} ${INVENTORY} ${KEY} ${DELAY} ${NAME} ${DRIVERS} ${EXTRA_IFACES}
# $- = shell options.
# $$ = Current shell PID.
# $0 = ?
# $1 = DEVPATH.
# $2 = INVENTORY
# $3 = KEY
# $4 = DELAY, or undefined?
# $5 = NAME
# $6 = DRIVERS.
# $7 = EXTRA_IFACES
# -z var - If the length of the string is zero.
# $# = number of arguments?
# /usr/bin/phosphor-gpio-presence --path=${DEVPATH} --inventory=${INVENTORY} --key=${KEY} --name=${NAME} --drivers=${DRIVERS} --extra-ifaces=${EXTRA_IFACES}
if [ -z "$4" ]; then
    cmd="/usr/bin/phosphor-gpio-presence --path=$1 --inventory=$2 --key=$3 --name=$5 --drivers=$6 --extra-ifaces=$7"
else
    cmd="/usr/bin/phosphor-gpio-presence --path=$1 --inventory=$2 --key=$3 --delay=$4 --name=$5 --drivers=$6 --extra-ifaces=$7"
fi

echo "Command: $cmd"

$cmd
