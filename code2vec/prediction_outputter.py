from interactive_predict import SHOW_TOP_CONTEXTS
from common import common
from code2vec import load_model_dynamically
from config import Config
import json
import numpy as np
from sklearn.decomposition import PCA
import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
import pickle
import sys
from argparse import ArgumentParser

# To Debug in VS code use in launch.json
#             "args": ["--load", "models/devign/saved_model_iter6.release", "--set-name", "wolf"],

sns.set()

def get_predictions(model, set_name, predictions_file, predictions_only=True):
    context_paths = "devign.%s.raw.txt"%set_name
    json_file = "../astminer/dataset/%s.jsonl"%set_name

    code_vectors = []
    labels = []

    with open(json_file) as sample_file, open(context_paths) as contexts_file, open(predictions_file, "w") as predictions:
        for sample, function in zip(sample_file, contexts_file):
            try:
                sample = json.loads(sample.strip())
                if not predictions_only:
                    predictions.write(f"\n===================:\n{sample['project']}\n{sample['func'][:200]}\n===================:\n")
                parts = function.rstrip().split(' ')
                method_name = parts[0]
                current_result_line_parts = [method_name]
                contexts = parts[1:]

                for context in contexts[:200]:
                    context_parts = context.split(',')
                    context_word1 = context_parts[0]
                    context_path = context_parts[1]
                    context_word2 = context_parts[2]
                    current_result_line_parts += ['%s,%s,%s' % (context_word1, context_path, context_word2)]
                space_padding = ' ' * (200 - len(contexts))
                result_line = ' '.join(current_result_line_parts) + space_padding
                raw_prediction_results = model.predict([result_line])
                method_prediction_results = common.parse_prediction_results(
                        raw_prediction_results,
                        model.vocabs.target_vocab.special_words, topk=SHOW_TOP_CONTEXTS)
                # for raw_prediction, method_prediction in zip(raw_prediction_results, method_prediction_results):
                #     predictions.write(f"{sample['idx']}\t{dicti[method_prediction.predictions[0]['name'][0]]}\n")
                for raw_prediction, method_prediction in zip(raw_prediction_results, method_prediction_results):
                    # predictions.write(f"{sample['idx']}\t{dicti[method_prediction.predictions[0]['name'][0]]}\n")
                    # Raw predictions contain attentions for different contexts
                    # predictions.write(f"{sample['project']}\t{sample['func'][:40]}\tRaw:{raw_prediction}\tMethod:{method_prediction.predictions}\n")
                    # predictions.write(f"{method_prediction.predictions}\n")
                    predictions.write(json.dumps(method_prediction.predictions)+"\n")
                    if not predictions_only:
                        predictions.write(' '.join(map(str, raw_prediction.code_vector)) + '\n')
                    code_vectors.append(raw_prediction.code_vector)
                    if not np.isnan(method_prediction.predictions[0]['probability']):
                        labels.append(method_prediction.predictions[0]['name'][0])
                    else:
                        labels.append("Unknown")
            except Exception as e:
                print(e)
                print(f"Error in sample: {sample}")
                print(f"Error in function: {function}")
        if not predictions_only:
            #count the labels of each class
            labels_counts = np.unique(np.array(labels), return_counts=True)
            predictions.write(str(labels_counts) + '\n')

    return labels,code_vectors

if __name__ == "__main__":
    config = Config(set_defaults=True, load_from_args=True, verify=True)
    config.EXPORT_CODE_VECTORS = True
    model = load_model_dynamically(config)

    arguments_parser = ArgumentParser()
    arguments_parser.add_argument('-sn', '--set-name', dest='set_name',
                        help="name of the dataset (used in prediction_outputter only)", required=True)
    arguments_parser.add_argument('-pf', '--predictions-file', dest='predictions_file',
                        help="the file to write the predictions to(used in prediction_outputter only)", required=True)

    try:
        args, _ = arguments_parser.parse_known_args()
    except Exception as e:
        print(e)

    labels,code_vectors = get_predictions(model, args.set_name, args.predictions_file)

    # # PCA visualization
    # pca = PCA(n_components=2)
    # X = np.nan_to_num(np.array(code_vectors))

    # pca = pca.fit(X)
    # if SET_NAME == "train":
    #     pickle.dump(pca, open("pca.pkl","wb"))
    # else:
    #     pca = pickle.load(open("pca.pkl",'rb'))

    # projected =  pca.transform(X)
    # print(X.shape)
    # print(projected.shape)

    # colors = {'FFmpeg':'red', 'openssl':'green', 'vlc':'blue'}
    # df = pd.DataFrame(dict(labels=labels, pca0=projected[:, 0], pca1 = projected[:, 1]))

    # # plt.scatter(projected[:, 0], projected[:, 1],
    # #             c=[colors[i] for i in labels], edgecolor='none', alpha=0.5,
    # #             # cmap=plt.cm.get_cmap('spectral', 10)
    # #             )
    # # plt.xlabel('component 1')
    # # plt.ylabel('component 2')
    # # plt.legend()

    # sns.scatterplot(x='pca0', y='pca1', data=df, hue='labels')
    # plt.xlim(-7, 7)
    # plt.ylim(-2.5, 7)
    # plt.savefig("PCA_%s.pdf"%SET_NAME, dpi=600, bbox_inches='tight')
    # plt.close()
