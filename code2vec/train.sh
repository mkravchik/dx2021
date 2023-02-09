#!/usr/bin/env bash
###########################################################
# Change the following values to train a new model.
# type: the name of the new model, only affects the saved file name.
# dataset: the name of the dataset, as was preprocessed using preprocess.sh
# test_data: by default, points to the validation set, since this is the set that
#   will be evaluated after each training iteration. If you wish to test
#   on the final (held-out) test set, change 'val' to 'test'.
# type=java14m
# dataset_name=java14m

NAMES=""
# For subtoken training

while getopts n flag
do
    case "${flag}" in
        n) NAMES=1;;
    esac
done

echo train NAMES $NAMES

if [ "$NAMES" != "" ]; then
    c2v_arg=--subtokens
    MAX_TOKEN_VOCAB_SIZE=100000
    MAX_TARGET_VOCAB_SIZE=100000 
    MAX_PATH_VOCAB_SIZE=100000 
    echo "WARNING! Changing the vocab sizes! Remember to restore!"
    sed -i -e "/self.MAX_TOKEN_VOCAB_SIZE =/ s/= .*/= ${MAX_TOKEN_VOCAB_SIZE}/" ./config.py
    sed -i -e "/self.MAX_TARGET_VOCAB_SIZE =/ s/= .*/= ${MAX_TARGET_VOCAB_SIZE}/" ./config.py
    sed -i -e "/self.MAX_PATH_VOCAB_SIZE =/ s/= .*/= ${MAX_PATH_VOCAB_SIZE}/" ./config.py
fi


type=devign
dataset_name=devign
data_dir=data/${dataset_name}
data=${data_dir}/${dataset_name}
test_data=${data_dir}/${dataset_name}.val.c2v
model_dir=models/${type}

mkdir -p ${model_dir}
python -u code2vec.py --data ${data} --test ${test_data} --save ${model_dir}/saved_model ${c2v_arg}
