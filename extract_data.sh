#!/bin/bash

# cp ./train.jsonl astminer/dataset/
# cp ./valid.jsonl astminer/dataset/

# echo Extracting ASTs
# cd astminer
# for split in train valid
# do
#     ./cli.sh ${split} 0
# done
# cd ..

echo Training Code2Vec
cd code2vec
source preprocess.sh
./train.sh
python3 code2vec.py --load models/devign/saved_model --release
cd ..