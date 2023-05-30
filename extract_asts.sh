#!/bin/bash

# if astminer/dataset direktory does not exist, create it
if [ ! -d "astminer/dataset" ]; then
    mkdir astminer/dataset
fi

cp ./$1.jsonl astminer/dataset/

# if the file astminer/dataset/$1.jsonl is newer than code2vec/devign.$1.raw txt file or there is no code2vec/devign.$1.raw txt, then we need to extract ASTs
if [ -f "code2vec/devign.$1.raw.txt" ] || [ "astminer/dataset/$1.jsonl" -nt "code2vec/devign.$1.raw.txt" ]; then
    echo Extracting ASTs from $1

    # Split the file into chunks of 10000 lines
    cd astminer/dataset
    split -l 10000 --numeric-suffixes=1 --suffix-length=2 --additional-suffix=".jsonl" "$1.jsonl" "$1."
    # Process each split file with cli.sh
    for file in $1.*.jsonl; do
        if [[ $file =~ $1\.[0-9]+\.jsonl ]]; then
            # Remove the .jsonl extension
            base=$(basename "$file" .jsonl)

            echo Extracting ASTs from $base
            cd ..
            ./cli.sh "$base" 0
            cd dataset
        fi
    done

    # Combine all output files into a single file
    cd ../../code2vec
    cat devign.$1.*.raw.txt > devign.$1.raw.txt

    # Remove the temporary raw files
    rm devign.$1.*.raw.txt

    cd ..
fi
