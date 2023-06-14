#!/bin/bash
echo Training Code2Vec
cd code2vec
# we do not need the test set, so we can use the 1 record of the train set
# if [ ! -f "devign.test.raw.txt" ]; then
#     head -n 1 ./devign.train.raw.txt > ./devign.test.raw.txt
# fi
source ./preprocess.sh
./train.sh
python3 code2vec.py --load models/devign/saved_model --release

# TEMP
python3 code2vec.py --load models/devign/saved_model.release --test data/devign/devign.test.c2v

cd ..