#!/bin/bash

cp ./train.jsonl astminer/dataset/
cp ./test.jsonl astminer/dataset/
cp ./valid.jsonl astminer/dataset/

STEP_SIZE=5
# Set SNIPPET_SIZE TO 0 to run on functions
for SNIPPET_SIZE in 10
do
    echo Running on code snippets of $SNIPPET_SIZE lines
    cd astminer
    for split in train valid test
    do
        ./cli.sh ${split} $SNIPPET_SIZE $STEP_SIZE
        cp ../code2vec/devign.${split}.raw.txt ../code2vec/devign.${split}.raw_backup.txt
        cp dataset/${split}.jsonl dataset/${split}_backup.jsonl
        cp dataset/${split}_lines.jsonl dataset/${split}_lines_backup.jsonl
    done
    cd ..
done
cd ../code2vec
source preprocess.sh