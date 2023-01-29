import os

cur_dir = (os.path.abspath(os.curdir))
os.chdir("..")
import subprocess
from sklearn.ensemble import RandomForestClassifier

import pickle
from tokenize_text import preprocess_data
import numpy as np
from feature_analysis import Doc2vec
from ClassMap import classMap
from typing import *
import json

mapper = classMap.mapper()
classes = mapper.getClasses()

os.chdir(cur_dir)


class C2VBoost:
    def __init__(self):

        self.model = RandomForestClassifier(n_estimators=1000,
                                            min_samples_split=0.001,
                                            max_features=2,
                                            class_weight="balanced",
                                            random_state=0
                                            )

        cur_dir = (os.path.abspath(os.curdir))
        os.chdir("..")
        pf = open("d2v.pickle", "rb")
        os.chdir(cur_dir)

        d2v: Doc2vec = pickle.load(pf)
        pf.close()
        self.feature_extractor = d2v
        self.text_preprocessor = preprocess_data
        self.classes = classes

    def fit(self, data):
        print(subprocess.run(
            "python ./cpp2jsonl.py -l ../sources -m ./ClassMap/classMap.json -jl " + data + " -sm -np",
            shell=True))
        print(subprocess.run("extract_data.sh", shell=True))
        print(subprocess.run(".code2vec/train.sh", shell=True))


    def predict(self, data):
        X = np.array(
            self.feature_extractor.to_vec_no_label(preprocess_data(data["func"]), data["syntactic_features"])).reshape(
            1, -1)
        if self.model is not None:
            probs = self.model.predict_proba(X)[0]
            if max(probs) < 0.6:
                return -1
            return self.model.predict(X)
        else:
            raise Exception("Model not Fitted")


if __name__ == "__main__":
    clf = C2VBoost()
    # Open the jsonl file
    data = []
    with open('refined_dataset.jsonl', 'r') as f:
        # Iterate over the lgit checkout -bines in the file
        for line in f:
            # Parse the line as JSON
            data.append(json.loads(line))
    # Train the model
    clf.fit(data)
    # Store the trained model on the disk
    file_to_store = open("trained_RF.pickle", "wb")
    pickle.dump(clf, file_to_store)