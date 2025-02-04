#!/bin/bash

LOOPS=1
NAMES=""

while getopts l:n flag
do
    case "${flag}" in
        l) LOOPS=${OPTARG};;
        n) NAMES=1;;
    esac
done

echo NAMES $NAMES

# assuming all previous stages were completed and we have ./{train, valid}.jsonl, ./{train, valid}_lines.jsonl in astminer/dataset/
# and devign.{train, valid}.raw.txt in code2vec
# running from the root directory
# PROJECTS_TO_DEL="fastsocket openssl"
# PROJECTS_TO_DEL="fastsocket cpprestsdk easyhttpcpp taox11"
PROJECTS_TO_DEL=""
for split in train valid #test
do
    # first restore the original files from the copies if we stopped in the middle
    cp code2vec/devign.${split}.raw_backup.txt code2vec/devign.${split}.raw.txt
    cp astminer/dataset/${split}_backup.jsonl astminer/dataset/${split}.jsonl
    cp astminer/dataset/${split}_lines_backup.jsonl astminer/dataset/${split}_lines.jsonl

    for PROJ_TO_DEL in $PROJECTS_TO_DEL
    do
        echo Found `grep \"project\"\:\"$PROJ_TO_DEL\" astminer/dataset/${split}_lines.jsonl  | wc -l` lines of $PROJ_TO_DEL in $split
        echo Removing $PROJ_TO_DEL from $split
        # filter out the $PROJ_TO_DEL from train and validation
        grep -n \"project\"\:\"$PROJ_TO_DEL\" astminer/dataset/${split}_lines.jsonl  | cut -d: -f1 > ${PROJ_TO_DEL}_line_nums
        sed 's%$%d%' ${PROJ_TO_DEL}_line_nums > ${PROJ_TO_DEL}_sed_del_lines
        sed -f ${PROJ_TO_DEL}_sed_del_lines code2vec/devign.${split}.raw.txt > code2vec/devign.${split}.raw.txt_
        mv code2vec/devign.${split}.raw.txt_ code2vec/devign.${split}.raw.txt
        # rm $PROJ_line_nums $PROJ_sed_del_lines

        grep -v \"project\"\:\"$PROJ_TO_DEL\" astminer/dataset/${split}_lines.jsonl  > astminer/dataset/${split}_lines.jsonl_
        mv astminer/dataset/${split}_lines.jsonl_ astminer/dataset/${split}_lines.jsonl
        echo Found `grep \"project\"\:\"$PROJ_TO_DEL\" astminer/dataset/${split}_lines.jsonl  | wc -l` lines of $PROJ_TO_DEL in $split

        cp astminer/dataset/${split}.jsonl astminer/dataset/${split}_copy.jsonl
        cp astminer/dataset/${split}_lines.jsonl astminer/dataset/${split}_lines_copy.jsonl
        cp code2vec/devign.${split}.raw.txt code2vec/devign.${split}.raw_copy.txt
    done

    # make sure we copy the full file aside so that we can move the projects out of it later.
    cp astminer/dataset/${split}.jsonl astminer/dataset/${split}_copy.jsonl
    cp astminer/dataset/${split}_lines.jsonl astminer/dataset/${split}_lines_copy.jsonl
    cp code2vec/devign.${split}.raw.txt code2vec/devign.${split}.raw_copy.txt

done

touch code2vec/res.csv
echo "project,class,precision,recall,f1-score,support" >> code2vec/res.csv

PROJECTS="7zip esp-idf poco qemu sumatrapdf fastsocket openssl vlc botan cryptopp httpp incubator-brpc cpprestsdk cpr DumaisLib easyhttpcpp obs-studio fineftp-server IXWebSocket libjson-rpc-cpp libtins nanomsg nghttp2 PcapPlusPlus restc-cpp taox11 uvw libtomcrypt imgui nana nanogui wxWidgets xtd qtbase libui JUCE" # gtk restbed grpc libashttp seastar sockpp tacopie"
#for project in $PROJECTS #7zip esp-idf poco qemu sumatrapdf vlc
for project in mbedtls qemu
do
    echo Moving $project from train to test
    echo "$project" >> code2vec/res.csv
    # remove the original test
    rm code2vec/devign.test.raw.txt 2>/dev/null
    rm astminer/dataset/test_lines_no_${project}.jsonl 2>/dev/null
    
    for split in train valid
    do
        grep -n \"project\"\:\"$project\" astminer/dataset/${split}_lines_copy.jsonl  | cut -d: -f1 > ${project}_line_nums
        # extract the test dataset
        sed 's%$%p%' ${project}_line_nums > ${project}_sed_choose_lines
        sed -n -f ${project}_sed_choose_lines code2vec/devign.${split}.raw_copy.txt >> code2vec/devign.test.raw.txt
        sed -n -f ${project}_sed_choose_lines astminer/dataset/${split}_lines_copy.jsonl >> astminer/dataset/test_lines_no_${project}.jsonl

        # # test 
        # echo The number of ${project} lines within astminer/dataset/test_lines_no_${project}.jsonl should be 0
        # grep -v \"project\"\:\"$project\" astminer/dataset/test_lines_no_${project}.jsonl  | wc -l

        # filter out the project from train and validation
        sed 's%$%d%' ${project}_line_nums > ${project}_sed_del_lines
        sed -f ${project}_sed_del_lines code2vec/devign.${split}.raw_copy.txt > code2vec/devign.${split}.raw.txt_
        mv code2vec/devign.${split}.raw.txt_ code2vec/devign.${split}.raw.txt

        # # test 
        sed -f ${project}_sed_del_lines astminer/dataset/${split}_lines_copy.jsonl > astminer/dataset/${split}_lines_no_${project}.jsonl
        # echo The number of ${project} lines within astminer/dataset/${split}_lines_no_${project}.jsonl should be 0
        # grep \"project\"\:\"$project\" astminer/dataset/${split}_lines_no_${project}.jsonl  | wc -l

        echo The total number of ${split} lines  `wc -l astminer/dataset/${split}_lines_copy.jsonl`
        echo The number of ${project} lines of ${split} `wc -l ${project}_sed_choose_lines`
        echo The total number of ${split} lines in c2v  `wc -l code2vec/devign.${split}.raw_copy.txt`
        echo The number of ${split} lines in c2v without ${project} `wc -l code2vec/devign.${split}.raw.txt`
        echo The number of ${split} jsonl lines `wc -l astminer/dataset/${split}_lines_no_${project}.jsonl`

        # rm ${split}_lines_no_${project}.jsonl 2>/dev/null
    done

    # rm astminer/dataset/test_lines_no_${project}.jsonl 2>/dev/null

    cd code2vec
    train_sh_arg=""
    c2v_arg=""
    if [ "$NAMES" != "" ]; then
        train_sh_arg=-n
        c2v_arg=--subtokens
    fi
    
    source preprocess.sh
    for (( i=1; i<=$LOOPS; i++ ))
    do
        echo Iteration $i of $project

        ./train.sh $train_sh_arg
        python3 code2vec.py --load models/devign/saved_model --release  ${c2v_arg}
        python3 code2vec.py --load models/devign/saved_model.release --test data/devign/devign.test.c2v  ${c2v_arg}
        # get the code vectors
        rm data/devign/devign.train.c2v.vectors 2>/dev/null
        python3 code2vec.py --load models/devign/saved_model.release --test data/devign/devign.train.c2v --export_code_vectors  ${c2v_arg}
        # python3 code2vec.py --load models/devign/saved_model.release --test data/devign/devign.val.c2v --export_code_vectors
        python3 code2vec.py --load models/devign/saved_model.release --test data/devign/devign.test.c2v --export_code_vectors  ${c2v_arg}

        # echo Random Forest including Unknown     
        # python3 ../c2v_vectors_rf.py --train data/devign/devign.train.c2v.vectors --trainjsonl ../astminer/dataset/train_lines_no_${project}.jsonl 
        # python3 ../c2v_vectors_rf.py --test data/devign/devign.val.c2v.vectors --testjsonl ../astminer/dataset/valid_lines_no_${project}.jsonl 
        # python3 ../c2v_vectors_rf.py --test data/devign/devign.test.c2v.vectors --testjsonl ../astminer/dataset/test_lines_no_${project}.jsonl
        
        echo Random Forest excluding Unknown    
        python3 ../c2v_vectors_rf.py --train data/devign/devign.train.c2v.vectors --trainjsonl ../astminer/dataset/train_lines_no_${project}.jsonl --no_unknown
        python3 ../c2v_vectors_rf.py --test data/devign/devign.test.c2v.vectors --testjsonl ../astminer/dataset/test_lines_no_${project}.jsonl --no_unknown

        # save the vectors aside - to see if we encode them differently
        mv data/devign/devign.train.c2v.vectors data/devign/devign.train.c2v.vectors.$project.$i
        mv data/devign/devign.test.c2v.vectors data/devign/devign.test.c2v.vectors.$project.$i
    done

    cd ..
    
    rm ${project}_line_nums ${project}_sed_del_lines ${project}_sed_choose_lines
done

# restore the original files
for split in train valid #test
do
    cp astminer/dataset/${split}_backup.jsonl astminer/dataset/${split}.jsonl
    cp astminer/dataset/${split}_lines_backup.jsonl astminer/dataset/${split}_lines.jsonl
    cp code2vec/devign.${split}.raw_backup.txt code2vec/devign.${split}.raw.txt
done
