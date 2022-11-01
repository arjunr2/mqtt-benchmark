import sys
from sklearn.cluster import KMeans
import numpy as np

def main():
    for line in sys.stdin.readlines():
        if line.startswith("Data Pts: "):
            line = line[10:]
            data = np.array(line.split(',')[:-1])
            num_pts = len(data)
            np_data = np.reshape(data, (-1, 1))
            kmeans = KMeans(n_clusters=3).fit(np_data)
            centers = np.sort(kmeans.cluster_centers_.flatten().astype(int))
            print("Min/Max: {}/{}".format(data[0], data[-1]))
            print("Q1/Q2/Q3: {}/{}/{}".format(*data[::num_pts//4][1:]))
            print("KMeans: {}/{}/{}".format(*centers))
        else:
            print(line, end="")

if __name__ == '__main__':
    main()
