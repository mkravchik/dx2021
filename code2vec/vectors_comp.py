from typing import List
import pandas as pd
import sys
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from sklearn.decomposition import PCA
import matplotlib.pyplot as plt
import seaborn as sns
import json

def load_vectors(vectors_file : str) -> pd.DataFrame:
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

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: python vectors_comp c2v.vectors-first-file c2v.vectors-second-file jsonl_with_labels")
        exit(1)
    df_data1 = load_vectors(sys.argv[1])
    df_data2 = load_vectors(sys.argv[2])
    if df_data1.shape[-1] != df_data2.shape[-1]:
        print("Usage: shapes should be the same.", df_data1.shape, df_data2.shape)
        exit(1)

    labels = load_labels(sys.argv[3])
    if df_data1.shape[0] != df_data2.shape[0] or df_data1.shape[0] != len(labels):
        print("Usage: number of elements should be the same.", df_data1.shape, df_data2.shape, len(labels))
        exit(1)

    # print("Heads")
    # print(df_data1.head().iloc[:,:15].to_string()) 
    # print(df_data2.head().iloc[:,:15].to_string()) 
    # print("Heads diffs")
    # print((df_data1 - df_data2).iloc[:,:15].abs().head().to_string()) 


    # print("Means:")
    # print(df_data1.mean(axis=0).to_string()) 
    # print(df_data2.mean(axis=0).to_string()) 

    # print("Means distance")
    # print(np.linalg.norm(df_data1.mean(axis=0) - df_data2.mean(axis=0)))

    # print("Differences norm")
    # print(np.linalg.norm((df_data1 - df_data2).values))
    
    size=8
    sns.set(rc={"font.size":size,"axes.titlesize":size,"axes.labelsize":size, "legend.fontsize":size, 'xtick.labelsize': size, 'ytick.labelsize': size},style="darkgrid")

    # find the last part of the file name
    last_dot = sys.argv[1].rfind(".")
    sec_last_dot = sys.argv[1].rfind(".", 0, last_dot-1)
    
    # df_data1.iloc[:,:12].hist()
    # plt.savefig(sys.argv[1][sec_last_dot+1:] + "_hist.pdf", dpi=600, bbox_inches='tight')
    # plt.close()

    # PCA visualization
    pca = PCA(n_components=2)
    projected = pca.fit_transform(df_data1)
    print(projected.shape)

    df = pd.DataFrame(dict(labels=labels, pca0=projected[:, 0], pca1 = projected[:, 1]))

    sns.scatterplot('pca0', 'pca1', data=df, hue='labels')
    plt.xlim(-7, 7)
    plt.ylim(-2.5, 7)
    plt.savefig("PCA_%s.pdf"%sys.argv[1][sec_last_dot+1:], dpi=600, bbox_inches='tight')
    plt.close()

    last_dot = sys.argv[2].rfind(".")
    sec_last_dot = sys.argv[2].rfind(".", 0, last_dot-1)
    # df_data2.iloc[:,:12].hist()
    # plt.savefig(sys.argv[2][sec_last_dot+1:] + "_hist.pdf", dpi=600, bbox_inches='tight')
    # plt.close()

    # PCA visualization
    pca = PCA(n_components=2)
    projected = pca.fit_transform(df_data2)
    print(projected.shape)

    df = pd.DataFrame(dict(labels=labels, pca0=projected[:, 0], pca1 = projected[:, 1]))

    sns.scatterplot('pca0', 'pca1', data=df, hue='labels')
    plt.xlim(-7, 7)
    plt.ylim(-2.5, 7)
    plt.savefig("PCA_%s.pdf"%sys.argv[2][sec_last_dot+1:], dpi=600, bbox_inches='tight')
    plt.close()
