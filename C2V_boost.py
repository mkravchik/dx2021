import os

cur_dir = (os.path.abspath(os.curdir))
os.chdir("..")
import subprocess
from sklearn.ensemble import RandomForestClassifier
from code2vec.prediction_outputter import get_prediction_output
from feature_analysis import Doc2vec
import pickle
import numpy as np
from ClassMap import classMap
from typing import *
import json

mapper = classMap.mapper()
classes = mapper.getClasses()

os.chdir(cur_dir)


class C2VBoost:
    def __init__(self):

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
            "python ./cpp2jsonl.py -l ../sources -m ./ClassMap/classMap.json -jl " + data + " -sm -np -s",
            shell=True))
        print(subprocess.run("extract_data.sh", shell=True))
        print(subprocess.run(".code2vec/train.sh", shell=True))


    def predict(self, data):

        labels = get_prediction_output("valid")[0]
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
    clf.predict('refined_dataset.jsonl')
    # Train the model
    #clf.fit('refined_dataset.jsonl')
