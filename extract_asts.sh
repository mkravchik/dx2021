#!/bin/bash

 cp ./$1.jsonl astminer/dataset/
 
 echo Extracting ASTs from $1
 cd astminer
 ./cli.sh $1 0
 cd ..