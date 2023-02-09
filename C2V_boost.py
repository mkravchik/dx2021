import os
import subprocess
# from code2vec.prediction_outputter import get_prediction_output
import numpy as np
from ClassMap import classMap
from typing import *
import json

mapper = classMap.mapper()
classes = mapper.getClasses()

class C2VBoost:
    def __init__(self):
        self.classes = classes

    def fit(self, data):
        # Splitting into train and validation (20%). No test.
        # print(subprocess.run(
        #     "python ./cpp2jsonl.py -l ../sources -m ./ClassMap/classMap.json -jl " + data + " -np -s -test 0 -af", shell=True))
        print(subprocess.run("./extract_data.sh", shell=True))
        # print(subprocess.run("./code2vec/train.sh", shell=True))


    def predict(self, data):
        # The get_prediction_output must be refactored to be run like that.
        # In its current form, it can't be even imported, it is supposed to be run and reads its arguments from the command line
        # labels = get_prediction_output("valid")[0]
        return labels


if __name__ == "__main__":
    clf = C2VBoost()
    clf.fit('refined_dataset.jsonl')
    # clf.predict('refined_dataset.jsonl')
