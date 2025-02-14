#!/bin/bash

START_ADDRESS=$1

END_ADDRESS=$(printf "0x%X" $(($START_ADDRESS + 0x20)))

objdump -D -M intel --start-address=$START_ADDRESS --stop-address=$END_ADDRESS kernel/bin-x86_64/kernel