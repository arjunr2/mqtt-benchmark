import sys
from sklearn.cluster import KMeans
import numpy as np

i = 0
for line in sys.stdin.readlines():
    if i == 17:
        data = np.array(line.split(',')[:-1])
        np_data = np.reshape(data, (-1, 1))
        kmeans = KMeans(n_clusters=3).fit(np_data)
        centers = np.sort(kmeans.cluster_centers_.flatten().astype(int))
        print("Clusters: {} - {} - {}".format(*centers))
    else:
        print(line, end="")
    i+=1
