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
echo "Number arguments: $#"
# [ "$#" -ne 7 ]
if [ "$#" -eq 7 ]; then
    echo "7 arguments. Has delay."
    echo "$0"
    echo "$1"
    echo "$2"
    echo "$3"
    echo "$4"
    echo "$5"
    echo "$6"
    echo "$7"
    if [ -z "$4" ]; then
        cmd="/usr/bin/phosphor-gpio-presence --path=$1 --inventory=$2 --key=$3 --name=$5 --drivers=$6 --extra-ifaces=$7"
    else
        cmd="/usr/bin/phosphor-gpio-presence --path=$1 --inventory=$2 --key=$3 --delay=$4 --name=$5 --drivers=$6 --extra-ifaces=$7"
    fi
    echo "Command: $cmd"
else
    echo "No delay."
    echo "$0"
    echo "$1"
    echo "$2"
    echo "$3"
    echo "$4"
    echo "$5"
    echo "$6"
    cmd="/usr/bin/phosphor-gpio-presence --path=$1 --inventory=$2 --key=$3 --name=$4 --drivers=$5 --extra-ifaces=$6"
    echo "Command: $cmd"
fi

$cmd
