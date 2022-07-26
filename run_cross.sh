#!/bin/bash

DATASET=/mnt/d/GitHub_Clones/scripts/C_Dataset/test
DATASET2=/mnt/d/GitHub_Clones/scripts/C_Dataset/test2
#DATASET=/home/tomerg1/git/sources
#DATASET2=/home/tomerg1/git/sources

# python ./cpp2jsonl.py -l $DATASET -np -m ./ClassMap/classMap.json -jl all_benchmark.jsonl -s -sm
# # NO SPLIT FOR THE TEST
# python ./cpp2jsonl.py -l $DATASET2 -m ./ClassMap/classMap.json -jl test.jsonl -sm

# cp ./train.jsonl astminer/dataset/
# cp ./test.jsonl astminer/dataset/
# cp ./valid.jsonl astminer/dataset/

# echo Running on code snippets
# cd astminer
# ./cli.sh train 10
# ./cli.sh test 10
# ./cli.sh valid 10

# assuming all previous stages were completed and we have ./{train, valid}.jsonl, ./{train, valid}_lines.jsonl in astminer/dataset/
# and devign.{train, valid}.raw.txt in code2vec
# running from the root directory

for split in train valid test
do
    cp astminer/dataset/${split}.jsonl astminer/dataset/${split}_copy.jsonl
    cp astminer/dataset/${split}_lines.jsonl astminer/dataset/${split}_lines_copy.jsonl
    cp code2vec/devign.${split}.raw.txt code2vec/devign.${split}.raw_copy.txt
done

# Remove FileManager from the data
for split in train valid
do
    echo Removing FileManager from $split
    # filter out the FileManager from train and validation
    grep -n \"label\"\:\"FileManager\" astminer/dataset/${split}_lines.jsonl  | cut -d: -f1 > FileManager_line_nums
    sed 's%$%d%' FileManager_line_nums > FileManager_sed_del_lines
    sed -f FileManager_sed_del_lines code2vec/devign.${split}.raw_copy.txt > code2vec/devign.${split}.raw.txt
    FileManager_line_nums FileManager_sed_del_lines

    grep -v \"label\"\:\"FileManager\" astminer/dataset/${split}_lines.jsonl  > astminer/dataset/${split}_lines.jsonl_
    mv astminer/dataset/${split}_lines.jsonl_ astminer/dataset/${split}_lines.jsonl
done

# now save aside the version with all the projects after removing the FileManager
for split in train valid test
do
    cp astminer/dataset/${split}_lines.jsonl astminer/dataset/${split}_lines_copy_nofm.jsonl
    cp code2vec/devign.${split}.raw.txt code2vec/devign.${split}.raw_copy_nofm.txt
done

# for project in 7zip esp-idf poco qemu sumatrapdf vlc
for project in 7zip 
do
    echo Moving $project to from train to test
    
    # remove the original test
    rm code2vec/devign.test.raw.txt
    rm astminer/dataset/test_lines_no_${project}.jsonl
    
    for split in train valid
    do
        # as we already altered the ${split}_lines.jsonl by removing the FileManager, we need to work with the updated
        # file, not with the original
        grep -n \"project\"\:\"$project\" astminer/dataset/${split}_lines_copy_nofm.jsonl  | cut -d: -f1 > ${project}_line_nums
        # extract the test dataset
        sed 's%$%p%' ${project}_line_nums > ${project}_sed_choose_lines
        sed -n -f ${project}_sed_choose_lines code2vec/devign.${split}.raw_copy_nofm.txt >> code2vec/devign.test.raw.txt
        sed -n -f ${project}_sed_choose_lines astminer/dataset/${split}_lines_copy_nofm.jsonl >> astminer/dataset/test_lines_no_${project}.jsonl

        # test 
        echo Should be 0
        grep -v \"project\"\:\"$project\" astminer/dataset/test_lines_no_${project}.jsonl  | wc -l

        # filter out the project from train and validation
        sed 's%$%d%' ${project}_line_nums > ${project}_sed_del_lines
        sed -f ${project}_sed_del_lines code2vec/devign.${split}.raw_copy_nofm.txt > code2vec/devign.${split}.raw.txt_
        mv code2vec/devign.${split}.raw.txt_ code2vec/devign.${split}.raw.txt

        #grep -v \"project\"\:\"$project\" astminer/dataset/${split}_lines_copy_nofm.jsonl  > astminer/dataset/${split}_lines_no_${project}.jsonl
        sed -f ${project}_sed_del_lines astminer/dataset/${split}_lines_copy_nofm.jsonl > astminer/dataset/${split}_lines_no_${project}.jsonl
        echo Should be 0
        grep \"project\"\:\"$project\" astminer/dataset/${split}_lines_no_${project}.jsonl  | wc -l

        rm ${split}_lines_no_${project}.jsonl
    done

    rm code2vec/devign.test.raw.txt astminer/dataset/test_lines_no_${project}.jsonl

    cd code2vec
    source preprocess.sh
    ./train.sh
    python3 code2vec.py --load models/devign/saved_model --release
    python3 code2vec.py --load models/devign/saved_model.release --test data/devign/devign.test.c2v

    rm ${project}_line_nums ${project}_sed_del_lines ${project}_sed_choose_lines
    cd ..

done

# restore the original files
for split in train valid test
do
    cp astminer/dataset/${split}_copy.jsonl astminer/dataset/${split}.jsonl
    cp astminer/dataset/${split}_lines_copy.jsonl astminer/dataset/${split}_lines.jsonl
    cp code2vec/devign.${split}.raw_copy.txt code2vec/devign.${split}.raw.txt
done

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