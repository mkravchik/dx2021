import string
from tokenize import String
from typing import List
import pandas as pd
import argparse
import json
from sklearn.metrics import classification_report, confusion_matrix
from sklearn.ensemble import RandomForestClassifier
import pickle
from sklearn.decomposition import PCA
import matplotlib.pyplot as plt
import seaborn as sns

def load_vectors(vectors_file : string) -> pd.DataFrame:
    return pd.read_csv(vectors_file, index_col=None, delim_whitespace=True, header=None)

def load_labels (labels_jsonl) -> List:
    labels = []
    # labels = [json.loads(line)["label"] for line in src.readline()]
    for line in open(labels_jsonl):
        try:
            labels.append(json.loads(line)["label"])
        except Exception as e:
            print("Skipping invalid line:", line, ".", e)
    return labels

def train_or_evaluate_model(vectors_file, labels_jsonl, model_name, train, header, show=False):
    df_data = load_vectors(vectors_file)
    labels = load_labels(labels_jsonl)

    print(header, df_data.shape, len(labels))
    if train:
        clf = RandomForestClassifier(class_weight='balanced')
        clf.fit(df_data, labels)
        pickle.dump(clf, open(model_name + ".mdl", 'wb'))
    else:
        clf = pickle.load(open(model_name + ".mdl", 'rb'))

    print(header + " metrics\n")
    pred_labels = clf.predict(df_data)
    print(confusion_matrix(labels, pred_labels ))
    print(classification_report(labels, pred_labels, zero_division=0))
    if (show):
        display(df_data, labels, pred_labels, header)

def display(df_vectors : pd.DataFrame, real_labels: List, pred_labels: List, name):
    # pca = PCA(n_components=2)
    # X = df_vectors

    # pca = pca.fit(X)
 
    # projected =  pca.transform(X)
    # print(X.shape)
    # print(projected.shape)

    # df = pd.DataFrame(dict(labels=labels, pca0=projected[:, 0], pca1 = projected[:, 1]))

    # sns.scatterplot('pca0', 'pca1', data=df, hue='labels')
    # plt.xlim(-7, 7)
    # plt.ylim(-2.5, 7)
    # plt.savefig("PCA_%s.pdf"%SET_NAME, dpi=600, bbox_inches='tight')
    # plt.close()
    print(name + " averages:\n")
    print(str(df_vectors.mean(axis=0)))

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("-tr", "--train", help="C2V vectors file location.")
    parser.add_argument("-trj", "--trainjsonl", help="C2V train jsonl file location. Used to extract the labels.")
    parser.add_argument("-ts", "--test", help="C2V test vectors file location.")
    parser.add_argument("-tsj", "--testjsonl", help="C2V test jsonl file location. Used to extract the labels.")
    parser.add_argument("-n", "--name", help="Model name.", default="rf_c2v")
    parser.add_argument("-d", "--display", help="Produce a map of 2-dimensional PCA of the vectors. Defaults to false.", action='store_true')

    args = parser.parse_args()
    print(args)
    if args.train is not None:
        train_or_evaluate_model(args.train, args.trainjsonl, args.name, True, "Train", args.display)
    if args.test is not None:
        train_or_evaluate_model(args.test, args.testjsonl, args.name, False, "Test", args.display)
    
