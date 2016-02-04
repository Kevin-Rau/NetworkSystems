#!/bin/bash

#'chmod u+x startServer.sh'
#depending on your system, run this with 'sudo' prior to ./startServer
echo "killing old processes..."
pkill -f proxy

make clean
make

./proxyServer 10001