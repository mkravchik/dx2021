import os
import subprocess
import numpy as np
from ClassMap import classMap
from typing import *
import json
import pickle
import json

mapper = classMap.mapper()
classes = mapper.getClasses()

class C2VBoost:
    def __init__(self, confidence_margin = 0.6):
        self.classes = classes
        self.confidence_margin = confidence_margin

    def fit(self, data):
        with open('data.jsonl', 'wt') as f:
            for item in data:
                json.dump(item, f)
                f.write('\n')
        # Splitting into train and validation (20%). No test.
        print(subprocess.run(
            "python ./cpp2jsonl.py -l ../sources -m ./ClassMap/classMap.json -jl data.jsonl -np -s -test 0 -af", shell=True))
        print(subprocess.run("./extract_asts.sh train", shell=True))
        print(subprocess.run("./extract_asts.sh valid", shell=True))
        print(subprocess.run("./train_code2vec.sh", shell=True))


    """
    Given a json with a function snippet returns its label. 
    The labels are indices of the class in the mapper's class list
    To confo
    """
    def predict(self, data):
        labels = []

        # 1. Write the samples into a file
        test_input_file = 'test.jsonl'
        with open(test_input_file, 'wt') as f:
            for item in data:
                if not 'full_func' in item:
                    print("The file does not contain full function body, run cpp2jsonl -af first")
                    return -1
                json.dump(item, f)
                f.write('\n')

        # 2. Extract the ast contexts
        print(subprocess.run("./extract_asts.sh test", shell=True))

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
                    probs[0]['name'][0] in classes:
                        labels.append(classes.index(probs[0]['name'][0]))
                else:
                    labels.append(-1)
        os.chdir("..")
        # 7. Return the result mapped to the index        
        return labels


if __name__ == "__main__":
    clf = C2VBoost()
    # Open the jsonl file
    data = []
    with open('refined_dataset.jsonl', 'r') as f:
        # Iterate over the lines in the file
        for line in f:
            # Parse the line as JSON
            data.append(json.loads(line))
    # Train the model
    clf.fit(data)
    # Store the trained model on the disk
    file_to_store = open("trained_C2V.pickle", "wb")
    pickle.dump(clf, file_to_store)

    labels = clf.predict(data[:10])
    print(labels)
