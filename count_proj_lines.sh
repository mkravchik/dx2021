#!/bin/bash
PROJECTS="7zip esp-idf poco qemu sumatrapdf fastsocket openssl vlc botan mbedtls cryptopp httpp" # incubator-brpc cpprestsdk cpr DumaisLib easyhttpcpp obs-studio fineftp-server grpc IXWebSocket libashttp libjson-rpc-cpp libtins nanomsg nghttp2 PcapPlusPlus restbed restc-cpp seastar sockpp tacopie taox11 uvw libtomcrypt imgui nana nanogui wxWidgets xtd qtbase libui JUCE gtk"
DIR=./astminer/dataset
for split in train valid # test
do
    FILE=${DIR}/${split}
    for project in $PROJECTS #7zip esp-idf poco qemu sumatrapdf vlc
    do
        # echo The number of ${project} lines within ${FILE}.jsonl
        # grep -E \"project\"\:[[:space:]]*\"$project\" ${FILE}.jsonl  | wc -l
        echo The number of ${project} lines within ${FILE}_lines.jsonl
        grep -E \"project\"\:[[:space:]]*\"$project\" ${FILE}_lines.jsonl  | wc -l
        # proj_lines=${DIR}/${split}_lines_no_$project.jsonl
        # if [ -f $proj_lines ]; then
        #     echo The number of ${project} lines within $proj_lines
        #     grep -E \"project\"\:[[:space:]]*\"$project\" $proj_lines | wc -l
        # fi
    done
done

# display_state () {
#     DIR=../dx2021
#     wc -l ${DIR}/code2vec/devign.$1.raw.txt
#     wc -l ${DIR}/astminer/dataset/$1.jsonl
#     wc -l ${DIR}/astminer/dataset/$1_lines.jsonl
#     wc -l ${DIR}/$1.jsonl
# }

# for split in train valid
# do
#     echo State of ${split}
#     display_state ${split}
# done
