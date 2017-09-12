#!/bin/sh
# Clipboard persistence manager startup script

if [ "x$AF_PIDDIR" = "x" ]; then
  echo "$0: Error, AF_PIDDIR is not defined"
  exit 2
fi
if [ "x$LAUNCHWRAPPER_NICE_TRYRESTART" = "x" ]; then
  echo "$0: Error, LAUNCHWRAPPER_NICE_TRYRESTART is not defined"
  exit 2
fi
if [ ! -w $AF_PIDDIR ]; then
  echo "$0: Error, directory $AF_PIDDIR is not writable"
  exit 2
fi
PROG=/usr/bin/clipboard-manager
SVC="clipboard-manager"

case "$1" in
start)  START=TRUE
        ;;
stop)   START=FALSE
        ;;
*)      echo "Usage: $0 {start|stop}"
	exit 1
        ;;
esac

if [ $START = TRUE ]; then
  source $LAUNCHWRAPPER_NICE_TRYRESTART start "$SVC" $PROG
else
  source $LAUNCHWRAPPER_NICE_TRYRESTART stop "$SVC" $PROG
fi
