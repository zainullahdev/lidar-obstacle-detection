// PCL lib Functions for processing point clouds 

#include "processPointClouds.h"
#include "kdtree.h"
#include "cluster.h"
#include <unordered_set>

//constructor:
template<typename PointT>
ProcessPointClouds<PointT>::ProcessPointClouds() {}


//de-constructor:
template<typename PointT>
ProcessPointClouds<PointT>::~ProcessPointClouds() {}


template<typename PointT>
void ProcessPointClouds<PointT>::numPoints(typename pcl::PointCloud<PointT>::Ptr cloud)
{
    std::cout << cloud->points.size() << std::endl;
}


template<typename PointT>
typename pcl::PointCloud<PointT>::Ptr ProcessPointClouds<PointT>::FilterCloud(typename pcl::PointCloud<PointT>::Ptr cloud, float filterRes, Eigen::Vector4f minPoint, Eigen::Vector4f maxPoint)
{

    // Time segmentation process
    auto startTime = std::chrono::steady_clock::now();

    // TODO:: Fill in the function to do voxel grid point reduction and region based filtering
    typename pcl::PointCloud<PointT>::Ptr cloud_filtered (new pcl::PointCloud<PointT>());
    pcl::VoxelGrid<PointT> sor;
    sor.setInputCloud(cloud);
    sor.setLeafSize(filterRes,filterRes,filterRes);
    sor.filter(*cloud_filtered);

    typename pcl::PointCloud<PointT>::Ptr cloud_region (new pcl::PointCloud<PointT>());;
    pcl::CropBox<PointT> cropBox(true);
    cropBox.setInputCloud(cloud_filtered);
    cropBox.setMin(minPoint);
    cropBox.setMax(maxPoint);
    cropBox.filter(*cloud_region);

    // create roof top box
    std::vector<int> indices;
    pcl::CropBox<PointT> roof(true);
    roof.setInputCloud(cloud_region);
    roof.setMin(Eigen::Vector4f (-1.5,-1.7,-1,1));
    roof.setMax(Eigen::Vector4f (2.6,1.7,-.4,1));
    roof.filter(indices);
    
    // convert indices to point indices
    // to be used by extract method 
    pcl::PointIndices::Ptr inliers (new pcl::PointIndices());
    for (int indice : indices)
    {
        inliers->indices.push_back(indice);
    } 

    pcl::ExtractIndices<PointT> extract;
    extract.setInputCloud(cloud_region);
    extract.setIndices(inliers);
    extract.setNegative(true);    
    extract.filter(*cloud_region);

    auto endTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "filtering took " << elapsedTime.count() << " milliseconds" << std::endl;

    return cloud_region;

}


template<typename PointT>
std::pair<typename pcl::PointCloud<PointT>::Ptr, typename pcl::PointCloud<PointT>::Ptr> ProcessPointClouds<PointT>::SeparateClouds(pcl::PointIndices::Ptr inliers, typename pcl::PointCloud<PointT>::Ptr cloud) 
{
  // TODO: Create two new point clouds, one cloud with obstacles and other with segmented plane

    typename pcl::PointCloud<PointT>::Ptr obsCloud (new pcl::PointCloud<PointT>());
    typename pcl::PointCloud<PointT>::Ptr roadCloud (new pcl::PointCloud<PointT>());

    for(int index: inliers-> indices)
    {
        roadCloud->points.push_back(cloud->points[index]);
    }

    pcl::ExtractIndices<PointT> filter;
    filter.setInputCloud(cloud);
    filter.setIndices(inliers);
    filter.setNegative(true);
    filter.filter(*obsCloud);
    std::pair<typename pcl::PointCloud<PointT>::Ptr, typename pcl::PointCloud<PointT>::Ptr> segResult(roadCloud,obsCloud);
    return segResult;
}



template<typename PointT>
std::pair<typename pcl::PointCloud<PointT>::Ptr, typename pcl::PointCloud<PointT>::Ptr> ProcessPointClouds<PointT>::SegmentPlane(typename pcl::PointCloud<PointT>::Ptr cloud, int maxIterations, float distanceThreshold)
{
    // Time segmentation process
    auto startTime = std::chrono::steady_clock::now();
    pcl::PointIndices::Ptr totalInliers (new pcl::PointIndices());
    std::unordered_set<int> inliersResult;
	srand(time(NULL));

    // For max iterations  
	// Randomly sample subset and fit line
	// Measure distance between every point and fitted line
	// If distance is smaller than threshold count it as inlier
	// pass indicies of inliers from fitted line with most inliers
    // to seperate function to seperate inliers from outliers

    for (int i=0; i< maxIterations; i++)
	{
	
		std::unordered_set<int> inliers;
		while(inliers.size() < 3)
			inliers.insert(rand()%cloud->points.size());

		float x1,y1,z1,x2,y2,z2,x3,y3,z3 = 0;

		auto itr = inliers.begin();

		x1 = cloud->points[*itr].x;
		y1 = cloud->points[*itr].y;
		z1 = cloud->points[*itr].z;
		itr++;
		x2 = cloud->points[*itr].x;
		y2 = cloud->points[*itr].y;
		z2 = cloud->points[*itr].z;
		itr++;
		x3 = cloud->points[*itr].x;
		y3 = cloud->points[*itr].y;
		z3 = cloud->points[*itr].z;

		float v1[3] = { (x2 - x1), (y2 - y1), (z2 - z1) };
		float v2[3] = { (x3 - x1), (y3 - y1), (z3 - z1) };
				
		float v1_v2[3] = { (v1[1] * v2[2]) - (v1[2] * v2[1]),
				   	 	   (v1[2] * v2[0]) - (v1[0] * v2[2]),
					 	   (v1[0] * v2[1]) - (v1[1] * v2[0])
				         };

		float A = v1_v2[0];
		float B = v1_v2[1];
		float C = v1_v2[2];
		float D = -((A * x1) + (B * y1) + (C * z1));

		for (int j=0; j < cloud->points.size(); j++)
		{
			if (inliers.count(j))
				continue;


			float x4 = cloud->points[j].x;
			float y4 = cloud->points[j].y;
			float z4 = cloud->points[j].z;

			float d = fabs(A * x4 + B * y4 + C * z4 + D) / sqrt(A*A + B*B + C*C);

			if (d < distanceThreshold)
				inliers.insert(j);
			
			if (inliers.size() > inliersResult.size())
				inliersResult = inliers;
		}

	}

    typename pcl::PointCloud<PointT>::Ptr  cloudInliers(new pcl::PointCloud<PointT>());
	typename pcl::PointCloud<PointT>::Ptr cloudOutliers(new pcl::PointCloud<PointT>());


    for(int index = 0; index < cloud->points.size(); index++)
	{
		if(inliersResult.count(index))
			cloudInliers->points.push_back(cloud->points[index]);
		else
			cloudOutliers->points.push_back(cloud->points[index]);
	}
    
    auto endTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "plane segmentation took " << elapsedTime.count() << " milliseconds" << std::endl;

    std::pair<typename pcl::PointCloud<PointT>::Ptr, typename pcl::PointCloud<PointT>::Ptr> segResult (cloudOutliers,cloudInliers);
     
    return segResult;
}



 
template<typename PointT>
std::vector<typename pcl::PointCloud<PointT>::Ptr> ProcessPointClouds<PointT>::Clustering(typename pcl::PointCloud<PointT>::Ptr cloud, float clusterTolerance, int minSize, int maxSize)
{

    // Time clustering process
    auto startTime = std::chrono::steady_clock::now();

    std::vector<typename pcl::PointCloud<PointT>::Ptr> clusters;
    std::vector<std::vector<float>> points;
    KdTree* tree = new KdTree;
    for (int i=0; i< cloud->points.size(); i++) 
    {
        std::vector<float> point = { 
            cloud->points[i].x, 
            cloud->points[i].y,
            cloud->points[i].z
        };
    	tree->insert(point,i); 
        points.push_back(point);
    }
   
    std::vector<std::vector<int>> eucClusters = euclideanCluster(points, tree, clusterTolerance);
    int clusterId = 0;
    for(std::vector<int> cluster : eucClusters)
  	{
  		typename pcl::PointCloud<PointT>::Ptr clusterCloud(new pcl::PointCloud<PointT>());
  		for(int indice: cluster)
        {
  		    clusterCloud->points.push_back(cloud->points[indice]);
        }
        clusterCloud->width = clusterCloud->points.size();
        clusterCloud->height = 1;
        clusterCloud->is_dense = true;
        clusters.push_back(clusterCloud);
  		clusterId++;
  	}

    auto endTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "clustering took " << elapsedTime.count() << " milliseconds and found " << clusters.size() << " clusters" << std::endl;

    return clusters;
}



template<typename PointT>
Box ProcessPointClouds<PointT>::BoundingBox(typename pcl::PointCloud<PointT>::Ptr cluster)
{

    // Find bounding box for one of the clusters
    PointT minPoint, maxPoint;
    pcl::getMinMax3D(*cluster, minPoint, maxPoint);

    Box box;
    box.x_min = minPoint.x;
    box.y_min = minPoint.y;
    box.z_min = minPoint.z;
    box.x_max = maxPoint.x;
    box.y_max = maxPoint.y;
    box.z_max = maxPoint.z;

    return box;
}


template<typename PointT>
void ProcessPointClouds<PointT>::savePcd(typename pcl::PointCloud<PointT>::Ptr cloud, std::string file)
{
    pcl::io::savePCDFileASCII (file, *cloud);
    std::cerr << "Saved " << cloud->points.size () << " data points to "+file << std::endl;
}


template<typename PointT>
typename pcl::PointCloud<PointT>::Ptr ProcessPointClouds<PointT>::loadPcd(std::string file)
{

    typename pcl::PointCloud<PointT>::Ptr cloud (new pcl::PointCloud<PointT>);

    if (pcl::io::loadPCDFile<PointT> (file, *cloud) == -1) //* load the file
    {
        PCL_ERROR ("Couldn't read file \n");
    }
    std::cerr << "Loaded " << cloud->points.size () << " data points from "+file << std::endl;

    return cloud;
}


template<typename PointT>
std::vector<boost::filesystem::path> ProcessPointClouds<PointT>::streamPcd(std::string dataPath)
{

    std::vector<boost::filesystem::path> paths(boost::filesystem::directory_iterator{dataPath}, boost::filesystem::directory_iterator{});

    // sort files in accending order so playback is chronological
    sort(paths.begin(), paths.end());

    return paths;

}