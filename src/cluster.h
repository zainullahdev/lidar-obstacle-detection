


inline void clusterHelper(int indice, const typename std::vector<std::vector<float>>& points,std::vector<int>& cluster, std::vector<bool>& processed, KdTree* tree,float distanceTol) 
{
	processed[indice] = true;
	cluster.push_back(indice);
	std::vector<int> nearby = tree->search(points[indice],distanceTol);
	for (int pointIndice : nearby)
	{
		if(!processed[pointIndice])
			clusterHelper(pointIndice,points,cluster,processed,tree,distanceTol);
	}
}


inline std::vector<std::vector<int>> euclideanCluster(const std::vector<std::vector<float>>& points, KdTree* tree, float distanceTol)
{
	std::vector<std::vector<int>> clusters;
	std::vector<bool> processed(points.size(),false);
	for (int i=0; i<points.size(); i++)
	{
		if(!processed[i])
		{
			std::vector<int> cluster;
			// check proximity
			clusterHelper(i,points,cluster,processed,tree,distanceTol);
			clusters.push_back(cluster);
		}
	}
	return clusters;
}
