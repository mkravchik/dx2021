#!/bin/bash

# if astminer/dataset directory does not exist, create it
if [ ! -d "astminer/dataset" ]; then
    mkdir astminer/dataset
fi

cp ./$1.jsonl astminer/dataset/

echo lines in input: $(wc -l $1.jsonl)

# if the file astminer/dataset/$1.jsonl is newer than code2vec/devign.$1.raw txt file or there is no code2vec/devign.$1.raw txt, then we need to extract ASTs
if [ -f "code2vec/devign.$1.raw.txt" ] || [ "./$1.jsonl" -nt "code2vec/devign.$1.raw.txt" ]; then
    echo Extracting ASTs from $1

    # Split the file into chunks of 10000 lines
    cd astminer/dataset
    rm $1.*.jsonl
    # Remove the temporary raw files
    rm ../../code2vec/devign.$1.*.raw.txt

    #Remove the existing split files
    rm $1.*.jsonl

    split -l 1000 --numeric-suffixes=1 --suffix-length=3 --additional-suffix=".jsonl" "$1.jsonl" "$1."
    # Process each split file with cli.sh
    loop=0
    for file in $1.*.jsonl; do
        if [[ $file =~ $1\.[0-9]+\.jsonl ]]; then
            # Remove the .jsonl extension
            base=$(basename "$file" .jsonl)

            echo Extracting ASTs from $base
            cd ..
            time ./cli.sh "$base" 0
            cd dataset
            loop=$((loop+1))
        fi
    done

    # Combine all _with_asts.jsonl files into a single file
    cat $1.*_with_asts.jsonl > $1.with_asts.jsonl
    
    # Combine all output files into a single file
    cd ../../code2vec

    # Combine the devign.$1.*.raw.txt files in to devign.$1.raw.txt maintaining the order of the files 
    #(e.g. devign.$1.001.raw.txt, devign.$1.002.raw.txt, etc.)
    cat devign.$1.*.raw.txt > devign.$1.raw.txt

    cd .. 
fi
