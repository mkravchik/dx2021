#!/bin/bash
echo Training Code2Vec
cd code2vec
source preprocess.sh
./train.sh
python3 code2vec.py --load models/devign/saved_model --release
cd ..