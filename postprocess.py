import sys
import re
import io
import printtools as pt
import numpy as np
from collections import OrderedDict
from sklearn.cluster import KMeans

def extract_results(log_str):
    offset = 16
    lines = log_str.splitlines()
    pat = re.compile(r"\[(hc-\d+)\]")

    headings = ["Device", "Mean", "Std Dev", "Min/Max", "Q1/Q2/Q3", "Kmeans"]
    agg_data = OrderedDict()
    for i, line in enumerate(lines):
        match = pat.match(line)
        if match:
            data_line = lines[i+offset]
            data_m = re.search(r"Data Pts: (.*)", data_line)
            node = match.group(1)
            data_node = np.array(data_m.group(1).split(',')[:-1]).astype(int)
            agg_data[node] = np.concatenate((agg_data.get(node, np.array([])), data_node))

    results = []
    for node, data in agg_data.items():
        kmeans = KMeans(n_clusters=3).fit(np.reshape(data, (-1, 1)))
        centers = np.sort(kmeans.cluster_centers_.flatten().astype(int))

        min_max = f"{int(data[0])}/{int(data[-1])}"
        quarts = '/'.join(data[::len(data)//4][1:].astype(int).astype(str))
        centers_str = '/'.join(centers.astype(str))
        result_node = [
            node,
            np.mean(data, dtype=int),
            np.std(data, dtype=int),
            min_max,
            quarts,
            centers_str
        ]
        results.append(result_node)


def main():
    for line in sys.stdin.readlines():
        if line.startswith("Data Pts: "):
            line = line[10:]
            data = np.array(line.split(',')[:-1]).astype(int)
            num_pts = len(data)
            np_data = np.reshape(data, (-1, 1))
            kmeans = KMeans(n_clusters=3).fit(np_data)
            centers = np.sort(kmeans.cluster_centers_.flatten().astype(int))
            #print("Std Dev: {}".format(int(np.std(data))))
            print("Min/Max: {}/{}".format(data[0], data[-1]))
            print("Q1/Q2/Q3: {}/{}/{}".format(*data[::num_pts//4][1:]))
            print("KMeans: {}/{}/{}".format(*centers))
        else:
            print(line, end="")

if __name__ == '__main__':
    main()
