#!/bin/bash

if [ $# -eq 0 ]
  then
    echo "Too few arguments"
    exit 1
fi

./Master $1 &
./Slave $1 3 &
./Slave $1 14 &
./Slave $1 15 &
./Slave $1 9 &
#./Slave $1 26 &
./Slave $1 5 &