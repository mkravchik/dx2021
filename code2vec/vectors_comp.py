import pandas as pd
import sys
import numpy as np

def load_vectors(vectors_file : str) -> pd.DataFrame:
    return pd.read_csv(vectors_file, index_col=None, delim_whitespace=True, header=None)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python vectors_comp c2v.vectors-first-file c2v.vectors-second-file")
        exit(1)
    df_data1 = load_vectors(sys.argv[1])
    df_data2 = load_vectors(sys.argv[2])
    if df_data1.shape[-1] != df_data2.shape[-1]:
        print("Usage: shapes should be the same.", df_data1.shape, df_data2.shape)
        exit(1)
    
    print("Heads")
    print(df_data1.head().iloc[:,:15].to_string()) 
    print(df_data2.head().iloc[:,:15].to_string()) 
    print("Heads diffs")
    print((df_data1 - df_data2).iloc[:,:15].abs().head().to_string()) 


    # print("Means:")
    # print(df_data1.mean(axis=0).to_string()) 
    # print(df_data2.mean(axis=0).to_string()) 

    print("Means distance")
    print(np.linalg.norm(df_data1.mean(axis=0) - df_data2.mean(axis=0)))

    print("Differences norm")
    print(np.linalg.norm((df_data1 - df_data2).values))