#!/bin/bash
for i in `seq 1 24`;
do
    let n=6000+$i
    ./server $i $n & sleep 2
    time ./game_socket $i $n
done