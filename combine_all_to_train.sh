display_state () {
    wc -l code2vec/devign.$1.raw.txt
    wc -l astminer/dataset/$1.jsonl
    wc -l astminer/dataset/$1_lines.jsonl
    wc -l ./$1.jsonl
}

echo The original state:

for split in train valid test
do
    # Restore
    cp code2vec/devign.${split}.map.raw_backup.txt code2vec/devign.${split}.raw.txt 
    cp astminer/dataset/${split}.map_backup.jsonl astminer/dataset/${split}.jsonl 
    cp astminer/dataset/${split}.map_lines_backup.jsonl astminer/dataset/${split}_lines.jsonl 
    cp ./${split}.map.jsonl ./${split}.jsonl 

    display_state ${split}
done

# Backup
# for split in train valid test
# do
#     cp code2vec/devign.${split}.raw.txt code2vec/devign.${split}.map.raw_backup.txt
#     cp astminer/dataset/${split}.jsonl astminer/dataset/${split}.map_backup.jsonl
#     cp astminer/dataset/${split}_lines.jsonl astminer/dataset/${split}.map_lines_backup.jsonl
#    cp ./${split}.jsonl ./${split}.map.jsonl
# done

# Consolidate everything into train
for split in valid test
do
    cat code2vec/devign.${split}.raw.txt >> code2vec/devign.train.raw.txt
    cat astminer/dataset/${split}.jsonl >> astminer/dataset/train.jsonl
    cat astminer/dataset/${split}_lines.jsonl >> astminer/dataset/train_lines.jsonl
    cat ./${split}.jsonl >> ./train.jsonl
done

# how many lines do we have in train
echo After moving everything into train
display_state train

# Split the train into train-validation 
mv ./train.jsonl ./all.jsonl # This can be done only once!

python ./cpp2jsonl.py -l ../sources -jl ./all.jsonl -s -np --test_ratio 0.0

cp ./train.jsonl astminer/dataset/
cp ./test.jsonl astminer/dataset/
cp ./valid.jsonl astminer/dataset/

echo After splitting the train.jsonl into train and validation
for split in train valid 
do
    display_state ${split}
done

# ls -lt ./*.jsonl

mv ./astminer/dataset/train_lines.jsonl ./astminer/dataset/all_lines.jsonl # This can be done only once!

# We need to get the lines so extract the same lines from raw.txt
python ./cpp2jsonl.py -l ../sources -jl ./astminer/dataset/all_lines.jsonl -s -np --test_ratio 0.0 -ln

cp code2vec/devign.train.raw.txt code2vec/devign.train_all.raw.txt # This can be done only once!

for split in train valid
do
    sed 's%$%p%' ${split}_line_nums > ${split}_sed_choose_lines
    sed -n -f ${split}_sed_choose_lines code2vec/devign.train_all.raw.txt > code2vec/devign.${split}.raw.txt
    cp ./${split}.jsonl astminer/dataset/${split}_lines.jsonl
done

echo After splitting the train_lines.jsonl into train and validation
for split in train valid 
do
    display_state ${split}
done

# Now save the state to the backup. Otherwise, run_cross will overwrite the created files from it
for split in train valid test
do
    cp code2vec/devign.${split}.raw.txt code2vec/devign.${split}.raw_backup.txt
    cp astminer/dataset/${split}.jsonl astminer/dataset/${split}_backup.jsonl
    cp astminer/dataset/${split}_lines.jsonl astminer/dataset/${split}_lines_backup.jsonl
done
