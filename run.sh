#!/bin/bash

DATASET=~/sources_train/ #../sources
DATASET2=..src/ #../sources
SNIPPET_SIZE=10

LOOPS=1
NAMES=""

while getopts l:n flag
do
    case "${flag}" in
        l) LOOPS=${OPTARG};;
        n) NAMES=--method-label;;
    esac
done

# UNCOMMENT the parts you want to run!

###############################   CPP -> JSONL ############################################################
# # If you don't want to re-parse the sourses add -np
# python ./cpp2jsonl.py -l $DATASET -m ./ClassMap/classMap.json -jl all_benchmark_full.jsonl -s -sm -np
# python ./cpp2jsonl.py -l $DATASET -m ./ClassMap/classMap.json -s  
python ./cpp2jsonl.py -l $DATASET -m ./ClassMap/classMap.json -s -sm -jl data.jsonl

#split 80-20
python ./cpp2jsonl.py -l $DATASET -m "/home/moshe/Fatal-Library/ClassMap/classMap.json" -jl data.jsonl -np -s -test 0

cp ~/Fatal-Library/Boosting/test.jsonl .

# # # NO SPLIT FOR THE TEST
# # python ./cpp2jsonl.py -l $DATASET2 -m ./ClassMap/classMap.json -jl test.jsonl -sm -np

cp ./train.jsonl astminer/dataset/
cp ./test.jsonl astminer/dataset/
cp ./valid.jsonl astminer/dataset/

###############################   JSONL -> c2v ############################################################
STEP_SIZE=5
# Set SNIPPET_SIZE TO 0 to run on functions
for SNIPPET_SIZE in 10 
do  
    echo Running on code snippets of $SNIPPET_SIZE lines
    cd astminer
    for split in train valid test
    do
        ./cli.sh ${split} $SNIPPET_SIZE $STEP_SIZE ${NAMES}
        cp ../code2vec/devign.${split}.raw.txt ../code2vec/devign.${split}.raw_backup.txt
        cp dataset/${split}.jsonl dataset/${split}_backup.jsonl
        cp dataset/${split}_lines.jsonl dataset/${split}_lines_backup.jsonl
    done
    cd ..
#    ./run_cross.sh "$@"
done


###############################   C2V         ############################################################
cd ./code2vec

train_sh_arg=""
c2v_arg=""
if [ "$NAMES" != "" ]; then
    train_sh_arg=-n
    c2v_arg=--subtokens
fi

source preprocess.sh
for (( i=1; i<=$LOOPS; i++ ))
do
    echo Iteration $i 

    ./train.sh $train_sh_arg
    
    python3 code2vec.py --load models/devign/saved_model --release  ${c2v_arg}
    # python3 code2vec.py --load models/devign/saved_model.release --test data/devign/devign.test.c2v
    # # get the code vectors
    # rm data/devign/devign.train.c2v.vectors data/devign/devign.val.c2v.vectors data/devign/devign.test.c2v.vectors 2>/dev/null
    python3 code2vec.py --load models/devign/saved_model.release --test data/devign/devign.train.c2v --export_code_vectors ${c2v_arg}
    python3 code2vec.py --load models/devign/saved_model.release --test data/devign/devign.val.c2v --export_code_vectors ${c2v_arg}
    python3 code2vec.py --load models/devign/saved_model.release --test data/devign/devign.test.c2v --export_code_vectors ${c2v_arg}
    python3 ../c2v_vectors_rf.py --train data/devign/devign.train.c2v.vectors --trainjsonl ../astminer/dataset/train_lines.jsonl --test data/devign/devign.val.c2v.vectors --testjsonl ../astminer/dataset/valid_lines.jsonl    
    python3 ../c2v_vectors_rf.py --train data/devign/devign.train.c2v.vectors --trainjsonl ../astminer/dataset/train_lines.jsonl --test data/devign/devign.test.c2v.vectors --testjsonl ../astminer/dataset/test_lines.jsonl    

    # # save the vectors aside - to see if we encode them differently
    # mv data/devign/devign.train.c2v.vectors data/devign/devign.train.c2v.vectors.$i
    # mv data/devign/devign.val.c2v.vectors data/devign/devign.val.c2v.vectors.$i
    # mv data/devign/devign.test.c2v.vectors data/devign/devign.test.c2v.vectors.$i
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