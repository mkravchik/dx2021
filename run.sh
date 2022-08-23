#!/bin/bash

DATASET=/mnt/d/GitHub_Clones/scripts/C_Dataset/test
DATASET2=/mnt/d/GitHub_Clones/scripts/C_Dataset/test2
#DATASET=/home/tomerg1/git/sources
#DATASET2=/home/tomerg1/git/sources
SNIPPET_SIZE=10

LOOPS=1

while getopts l: flag
do
    case "${flag}" in
        l) LOOPS=${OPTARG};;
    esac
done

# If you don't want to re-parse the sourses add -np
python ./cpp2jsonl.py -l $DATASET -m ./ClassMap/classMap.json -jl all_benchmark.jsonl -s -sm -np

# NO SPLIT FOR THE TEST
python ./cpp2jsonl.py -l $DATASET2 -m ./ClassMap/classMap.json -jl test.jsonl -sm -np

cp ./train.jsonl astminer/dataset/
cp ./test.jsonl astminer/dataset/
cp ./valid.jsonl astminer/dataset/

for SNIPPET_SIZE in 10 
do  
    echo Running on code snippets of $SNIPPET_SIZE lines
    cd astminer
    # for split in test
    for split in train valid test
    do
        ./cli.sh ${split} $SNIPPET_SIZE
        cp ../code2vec/devign.${split}.raw.txt ../code2vec/devign.${split}.raw_backup.txt
        cp dataset/${split}.jsonl astminer/dataset/${split}_backup.jsonl
        cp dataset/${split}_lines.jsonl astminer/dataset/${split}_lines_backup.jsonl
    done
    
    cd ..
    source ./run_cross.sh
done


cd ./code2vec
source preprocess.sh
for (( i=1; i<=$LOOPS; i++ ))
do
    ./train.sh
    python3 code2vec.py --load models/devign/saved_model --release
    python3 code2vec.py --load models/devign/saved_model.release --test data/devign/devign.test.c2v
    # get the code vectors
    rm data/devign/devign.train.c2v.vectors data/devign/devign.val.c2v.vectors data/devign/devign.test.c2v.vectors 2>/dev/null
    python3 code2vec.py --load models/devign/saved_model.release --test data/devign/devign.train.c2v --export_code_vectors
    python3 code2vec.py --load models/devign/saved_model.release --test data/devign/devign.val.c2v --export_code_vectors
    python3 code2vec.py --load models/devign/saved_model.release --test data/devign/devign.test.c2v --export_code_vectors

    # save the vectors aside - to see if we encode them differently
    mv data/devign/devign.train.c2v.vectors data/devign/devign.train.c2v.vectors.$i
    mv data/devign/devign.val.c2v.vectors data/devign/devign.val.c2v.vectors.$i
    mv data/devign/devign.test.c2v.vectors data/devign/devign.test.c2v.vectors.$i
done

cd ..



# echo Running on functions
# cd astminer
# ./cli.sh train
# ./cli.sh test
# ./cli.sh valid
# cd ../code2vec
# source preprocess.sh
# ./train.sh
# python3 code2vec.py --load models/devign/saved_model --release
# python3 code2vec.py --load models/devign/saved_model.release --test data/devign/devign.test.c2v
# cd ..

# echo Training on functions, validation on snippets
# cd astminer
# ./cli.sh train 
# ./cli.sh test 10
# ./cli.sh valid 10
# cd ../code2vec
# source preprocess.sh
# ./train.sh
# python3 code2vec.py --load models/devign/saved_model --release
# python3 code2vec.py --load models/devign/saved_model.release --test data/devign/devign.test.c2v
# cd ..