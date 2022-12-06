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
# import matplotlib.pyplot as plt
# import seaborn as sns

def load_vectors(vectors_file : string) -> pd.DataFrame:
    return pd.read_csv(vectors_file, index_col=None, delim_whitespace=True, header=None)

def load_labels (labels_jsonl) -> List:
    labels = []
    # labels = [json.loads(line)["label"] for line in src.readline()]
    for line in open(labels_jsonl):
        try:
            labels.append(json.loads(line)["map_label"])
        except Exception as e:
            print("Skipping invalid line:", line, ".", e)
    return labels

def train_or_evaluate_model(vectors_file, labels_jsonl, model_name, train, header, show=False, remove_unknown=False, max_feats="auto"):
    df_data = load_vectors(vectors_file)
    labels = load_labels(labels_jsonl)
    df_labels = pd.DataFrame(labels)
    if remove_unknown:
        drop_idx = df_labels[ df_labels[0] == 'Unknown' ].index
        df_data.drop(drop_idx, inplace=True)
        df_labels.drop(drop_idx, inplace=True)

    print(header, df_data.shape, df_labels.shape)
    if train:
        clf = RandomForestClassifier(class_weight='balanced', max_features=max_feats)
        clf.fit(df_data, df_labels[0])
        pickle.dump(clf, open(model_name + ".mdl", 'wb'))
    else:
        clf = pickle.load(open(model_name + ".mdl", 'rb'))

    print(header + " metrics\n")
    pred_labels = clf.predict(df_data)
    print(confusion_matrix(df_labels[0], pred_labels ))
    print(classification_report(df_labels[0], pred_labels, zero_division=0))
    if (show):
        display(df_data, df_labels[0], pred_labels, header)

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
    parser.add_argument("-nu", "--no_unknown", help="Remove the Unknown class from evaluation.", action='store_true')
    parser.add_argument("-mf", "--max_features", help="The number of features to consider when looking for the best split", type=int, default=0)

    args = parser.parse_args()
    print(args)
    max_features = "auto" if args.max_features == 0 else args.max_features
    if args.train is not None:
        train_or_evaluate_model(args.train, args.trainjsonl, args.name, True, "Train", args.display, args.no_unknown, max_features)
    if args.test is not None:
        train_or_evaluate_model(args.test, args.testjsonl, args.name, False, "Test", args.display, args.no_unknown, max_features)
    
