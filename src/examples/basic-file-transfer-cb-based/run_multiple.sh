#!/bin/bash
cd ../../../
source env.bash
cd -
for i in {1..10}; do
    ./client_file_transfer.exe  > "client_$i.log" 2>&1 &
done

wait