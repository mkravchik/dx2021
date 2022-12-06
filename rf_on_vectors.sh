#!/bin/bash

# DATASET=/mnt/d/GitHub_Clones/scripts/C_Dataset/test
# DATASET2=/mnt/d/GitHub_Clones/scripts/C_Dataset/test2
DATASET=../sources
DATASET2=../sources


# PROJECTS="mbedtls 7zip esp-idf poco qemu sumatrapdf fastsocket openssl vlc botan cryptopp httpp incubator-brpc cpprestsdk cpr DumaisLib easyhttpcpp obs-studio fineftp-server grpc IXWebSocket libashttp libjson-rpc-cpp libtins nanomsg nghttp2 PcapPlusPlus restbed restc-cpp seastar sockpp tacopie taox11 uvw libtomcrypt imgui nana nanogui wxWidgets xtd qtbase libui JUCE gtk"
PROJECTS="7zip esp-idf poco" #  qemu sumatrapdf fastsocket openssl vlc botan mbedtls cryptopp httpp" 
for project in $PROJECTS 
do
    echo Testing RF on $project

    cd code2vec
    
    for i in 2 4
    do
        echo Testing $i RF features
        python3 ../c2v_vectors_rf.py --train data/devign/devign.train.c2v.vectors.$project.1 --max_features $i --trainjsonl ../astminer/dataset/train_lines_no_${project}.jsonl --no_unknown --test data/devign/devign.test.c2v.vectors.$project.1 --testjsonl ../astminer/dataset/test_lines_no_${project}.jsonl --no_unknown
    done
    cd ..
done


