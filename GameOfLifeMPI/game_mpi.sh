#!/bin/bash
for i in `seq 3 25`;
do
	time mpiexec -np $i ./game_mpi
done