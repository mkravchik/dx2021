#!/bin/bash

preper_data () {
	./cli.sh parse --lang cpp --project 7zip_cpp_split/p/$1 --output 7zip_cpp_split/o/$1 --storage dot
	./cli.sh pathContexts --lang cpp --project 7zip_cpp_split/p/$1 --output 7zip_cpp_split/o/$1
	./cli.sh code2vec --lang cpp --project 7zip_cpp_split/p/$1 --output 7zip_cpp_split/o/$1 --split-tokens --granularity method
}

./cli.sh preprocess --project 7zip_cpp_split/src --output 7zip_cpp_split/p

preper_data "test"
preper_data "train"
preper_data "val"