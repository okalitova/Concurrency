#!/bin/bash
for i in `seq 1 24`;
do
	time ./game_openmpi $i
done
