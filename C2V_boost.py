import argparse
import os
import subprocess
import time
import numpy as np
from ClassMap import classMap
from typing import *
import json
import pickle
import json
from tqdm import tqdm
from sklearn.metrics import classification_report, confusion_matrix

import sys
#sys.path.append(os.getcwd())
#sys.path.append(os.path.realpath(os.path.dirname(__file__)))
# os.chdir("..")
# cur_dir = (os.path.abspath(os.curdir))
# os.chdir(os.path.realpath(os.path.dirname(__file__)))

# mapper = classMap.mapper()
# classes = mapper.getClasses()

"""
Compare two files and return True if they are the same, False otherwise
"""
def compare_files(file1, file2):
    # first check if the files have the same size
    if os.path.getsize(file1) != os.path.getsize(file2):
        print(f"The files have different sizes {file1} {os.path.getsize(file1)} {file2} {os.path.getsize(file2)}")
        return False
    line_cnt = 0
    with open(file1, 'r') as f1:
        with open(file2, 'r') as f2:
            for line1 in f1:
                line_cnt += 1
                for line2 in f2:
                    if line1 != line2:
                        print(f"The files have different lines {line_cnt} {line1} {line2}")
                        return False
                    break # break from the inner loop, read next line from both files
    return True


"""
If the test_input_file and org_input_file are different, then we need to overwrite the org_input_file with test_input_file
Returns True if the files are different and the org_input_file was overwritten, False otherwise
"""
def overwrite_if_needed(test_input_file, org_input_file):
    if not compare_files(test_input_file, org_input_file):
        print(subprocess.run("mv " + test_input_file + " " + org_input_file, shell=True))
        return True
    else:
        # remove the test_input_file
        print(subprocess.run("rm " + test_input_file, shell=True))
    return False

class C2VBoost:
    def __init__(self, confidence_margin = 0.6, sources_dir = "../sources", classMap_path = "./ClassMap/classMap.json"):
        self.sources = os.path.abspath(sources_dir)
        self.classMap_path = os.path.abspath(classMap_path)
        self.classes = classMap.mapper(classMap_path).getClasses() #classes
        self.confidence_margin = confidence_margin

    def fit(self, data):
        # As we are running scripts from the code2vec directory, we need to change the directory
        # Save the current directory first
        cur_dir = (os.path.abspath(os.curdir))
        os.chdir(os.path.realpath(os.path.dirname(__file__)))
        print("Running from ", os.getcwd())
        with open('data.jsonl', 'wt') as f:
            for item in data:
                json.dump(item, f)
                f.write('\n')
        # Splitting into train and validation (20%). No test.
        print(subprocess.run(
            f"python ./cpp2jsonl.py -l {self.sources} -m {self.classMap_path} -jl data.jsonl -np -s -test 0 -trl train_tmp.jsonl -vl valid_tmp.jsonl", shell=True))

        # # 2. Extract the ast contexts
        if overwrite_if_needed("train_tmp.jsonl", 'train.jsonl'):
            print(subprocess.run("./extract_asts.sh train", shell=True))
        else:
            print("Using the existing train.jsonl and its ASTs")

        if overwrite_if_needed("valid_tmp.jsonl", 'valid.jsonl'):
            print(subprocess.run("./extract_asts.sh valid", shell=True))
        else:
            print("Using the existing valid.jsonl and its ASTs")
        
        print(subprocess.run("./train_code2vec.sh", shell=True))

        #restore the directory
        os.chdir(cur_dir)


    """
    Given a json with a function snippet returns its label. 
    The labels are indices of the class in the mapper's class list
    To confo
    """
    def predict(self, data):
        # As we are running scripts from the code2vec directory, we need to change the directory
        # Save the current directory first
        cur_dir = (os.path.abspath(os.curdir))
        os.chdir(os.path.realpath(os.path.dirname(__file__)))

        labels = []

        # 1. Write the samples into a file
        test_input_file = 'test_tmp.jsonl'
        with open(test_input_file, 'wt') as f:
            for item in data:
                # if not 'full_func' in item:
                #     print("The file does not contain full function body, run cpp2jsonl -af first")
                #     return -1
                json.dump(item, f)
                f.write('\n')

        
        # 2. Extract the ast contexts
        if overwrite_if_needed(test_input_file, 'test.jsonl'):
            print(subprocess.run("./extract_asts.sh test", shell=True))
            # warn if there is a different number of lines in test.jsonl and astminer/dataset/test_with_asts.jsonl
            if not compare_files("test.jsonl", "astminer/dataset/test_with_asts.jsonl"):
                print("The number of lines in test.jsonl and astminer/dataset/test_with_asts.jsonl is different!\
                       For compatibility with other models, the test.jsonl should be overwritten with the test_with_asts.jsonl")
        else:
            print("Using the existing test.jsonl and its ASTs")

        # 3. Preprocess for code2vec
        os.chdir("code2vec")
        # Preprocessing only the test dataset 
        print(subprocess.run("./preprocess.sh -t", shell=True))

        # 4. Get the predictions into predictions_test.txt
        predictions_file = "preds.jsonl"
        print(subprocess.run(
            "python ./prediction_outputter.py --load models/devign/saved_model.release --set-name test --predictions-file " + predictions_file, shell=True))
        # 5. Read the probabilities
        with open(predictions_file, 'r') as f:
            # Iterate over the lines in the file
            for line in f:
                # Parse the line as JSON
                probs = json.loads(line.strip())
        # 6. Compare the probability with the margin
                if len(probs) and \
                    probs[0]['probability'] > self.confidence_margin and \
                    probs[0]['name'][0] in self.classes:
                        labels.append(self.classes.index(probs[0]['name'][0]))
                else:
                    labels.append(-1)
        os.chdir("..")

        #restore the directory
        os.chdir(cur_dir)

        # 7. Return the result mapped to the index        
        return labels


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-tr", "--train_jsonl", help="Train jsonl location. Defaults to %s." % 'refined_dataset.jsonl',
                        default='refined_dataset.jsonl')
    parser.add_argument("-ts", "--test_jsonl", help="Test jsonl location. Defaults to %s." % "refined_test.jsonl",
                        default="refined_test.jsonl")
    args = parser.parse_args()
    print(args)

    clf = C2VBoost()
    # Open the jsonl file
    data = []
    with open(args.train_jsonl, 'r') as f:
        # Iterate over the lines in the file
        for line in f:
            # Parse the line as JSON
            data.append(json.loads(line))

    start_time = time.time()

    # Train the model
    clf.fit(data)
    # Store the trained model on the disk
    file_to_store = open("trained_C2V.pickle", "wb")
    pickle.dump(clf, file_to_store)

    print("Training time: ", time.time() - start_time)
    #labels = clf.predict(data[:10])
    #print(labels)
    
    # Test the model
    labels = []
    pred_labels = []
    data = []
    total_lines = 0
    with open(args.test_jsonl, 'r') as f:
        # Iterate over the lines in the file
        for line in f:
            if line.isspace():
                continue
            total_lines += 1
            data.append(json.loads(line))
    
    start_time = time.time()
    predictions = clf.predict(data)
    for i in range(len(predictions)):
        if predictions[i] != -1:
            pred_labels.append(clf.classes[predictions[i]])
            labels.append(data[i]["label"])

    print(f"Models agreed on {len(labels)} out of {total_lines}")
    print(confusion_matrix(labels, pred_labels ))
    print(classification_report(labels, pred_labels, zero_division=0))
    print("Testing time: ", time.time() - start_time)
