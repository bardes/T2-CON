#!/usr/bin/env bash

PROGRAM=./filter
TESTS=10
THREADS="1 2 3 4"
FILES=$(find ./imgs -name '*.p[gp]m')

echo -e "#File\tThreads\tTime\tStdDev" > results
for N_THREADS in $THREADS;
do
    for FILE in $FILES;
    do
        printf "Calculating %s with %d threads.\t" $FILE $N_THREADS
        truncate -s0 .tmp_results
        for ((_itr=0;_itr<$TESTS;_itr++))
        do
            OMP_NUM_THREADS=$N_THREADS /usr/bin/time -f '%e' $PROGRAM $FILE /dev/null 2>>.tmp_results;
            echo -n '#'
        done;
        echo
        echo -ne "$FILE\t$N_THREADS\t" >> results

        #Some awkfu to average out the results and get the deviation
        awk '{s+=$1}END{printf "%f\t", s/NR}' RS='\n' .tmp_results >> results
        awk '{sum+=$1; sumsq+=$1*$1} END {print sqrt(sumsq/NR - (sum/NR)**2)}' .tmp_results >> results
    done;
done;

rm -f .tmp_results
