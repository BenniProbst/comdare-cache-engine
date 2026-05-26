#!/bin/bash

export REPETITIONS=16

datasets=( "enwiki-20190901-all-titles.shuf" "random.shuf" "sf100_p_name.shuf" "sf10_p_name.shuf" "sf1_p_name.shuf" )
for dataset in "${datasets[@]}"
do
    ./bart "data/shuffled/$dataset" >> out.csv
    ./pbtreestatic "data/shuffled/$dataset" >> out.csv
    ./treeception "data/shuffled/$dataset" >> out.csv
done
