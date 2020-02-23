#!/bin/sh
IP=$(ifconfig en0 | grep inet | awk '$1=="inet" {print $2}')
issocat=$(lsof -i:6000| grep socat| wc -l)
pros=$(lsof -i:6000|wc -l)
int1=1
int2=2
if [ -z "$DISPLAY" ]
then
    echo "Can't detect X11 desplay. If you can't detect where is your display, check ./doc/xhost_trouble_shooting.md"
    exit
else
    # xterm
    xterm -e "$(xhost $IP)"
fi
socat TCP-LISTEN:6000,reuseaddr,fork UNIX-CLIENT:\"$DISPLAY\" &
docker run --rm -it -v "$(pwd):/root/models" -u "$(id -u):$(id -g)" -u=0 -v /tmp/.X11-unix:/tmp/.X11-unix -e DISPLAY="$IP:0" omnetpp/omnetpp-gui:u18.04-5.6.1
trap "lsof -i:6000 -t|xargs kill" 0