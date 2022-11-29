#!/bin/bash
DIR=. # ./astminer/dataset
KEY="map_label"
SUFFIX= #_lines
PROJECTS="7zip esp-idf poco qemu sumatrapdf fastsocket openssl vlc botan mbedtls cryptopp httpp" # incubator-brpc cpprestsdk cpr DumaisLib easyhttpcpp obs-studio fineftp-server grpc IXWebSocket libashttp libjson-rpc-cpp libtins nanomsg nghttp2 PcapPlusPlus restbed restc-cpp seastar sockpp tacopie taox11 uvw libtomcrypt imgui nana nanogui wxWidgets xtd qtbase libui JUCE gtk"

for split in train valid # test
do
    FILE=${DIR}/${split}
    for label in Unknown GUI crypto network #7zip esp-idf poco qemu sumatrapdf vlc
    do
        echo The number of ${label} lines within ${FILE}$SUFFIX.jsonl
        grep -E \"$KEY\"\:[[:space:]]*\"$label\" ${FILE}$SUFFIX.jsonl  > /tmp/${split}_${label} && cat /tmp/${split}_${label} | wc -l
        for project in $PROJECTS #7zip esp-idf poco qemu sumatrapdf vlc
        do
            echo The number of ${project} lines within ${FILE}$SUFFIX.jsonl mapped as ${label}
            grep -E \"project\"\:[[:space:]]*\"$project\" /tmp/${split}_${label}  | wc -l
        done
    done
done


# for split in train # valid # test
# do
#     FILE=${DIR}/${split}
#     for project in $PROJECTS #7zip esp-idf poco qemu sumatrapdf vlc
#     do
#         # echo The number of ${project} lines within ${FILE}.jsonl
#         # grep -E \"project\"\:[[:space:]]*\"$project\" ${FILE}.jsonl  | wc -l
#         echo The number of ${project} lines within ${FILE}$SUFFIX.jsonl
#         grep -E \"project\"\:[[:space:]]*\"$project\" ${FILE}$SUFFIX.jsonl  | wc -l
#         # proj$SUFFIX=${DIR}/${split}$SUFFIX_no_$project.jsonl
#         # if [ -f $proj$SUFFIX ]; then
#         #     echo The number of ${project} lines within $proj$SUFFIX
#         #     grep -E \"project\"\:[[:space:]]*\"$project\" $proj$SUFFIX | wc -l
#         # fi
#     done
# done

# display_state () {
#     DIR=../dx2021
#     wc -l ${DIR}/code2vec/devign.$1.raw.txt
#     wc -l ${DIR}/astminer/dataset/$1.jsonl
#     wc -l ${DIR}/astminer/dataset/$1$SUFFIX.jsonl
#     wc -l ${DIR}/$1.jsonl
# }

# for split in train valid
# do
#     echo State of ${split}
#     display_state ${split}
# done
