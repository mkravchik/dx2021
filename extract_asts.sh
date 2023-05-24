#!/bin/bash

# if astminer/dataset direktory does not exist, create it
if [ ! -d "astminer/dataset" ]; then
    mkdir astminer/dataset
fi

cp ./$1.jsonl astminer/dataset/

echo Extracting ASTs from $1
cd astminer
./cli.sh $1 0
cd ..