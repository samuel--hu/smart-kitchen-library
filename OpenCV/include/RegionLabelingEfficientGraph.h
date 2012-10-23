﻿/*!
 * @file RegionLabelingEfficientGraph.h
 * @author a_hasimoto
 * @date Date Created: 2012/Oct/07
 * @date Last Change:2012/Oct/22.
 */
#ifndef __SKL_REGION_LABELING_EFFICIENT_GRAPH_H__
#define __SKL_REGION_LABELING_EFFICIENT_GRAPH_H__


#include "FilterMat2Mat.h"


namespace skl{

/*!
 * @class "Efficient Graph-Based Image Segmentation"に基づく領域分割
 * @note "Efficient Graph-Based Image Segmentation," Pedro F. Felzenszwalb and Daniel P. Huttenlocher, International Journal of Computer Vision, 59(2) September 2004.
 * @example ../samples/Segmentation/sample_segmentation_efficient_graph.cpp
 */
 class RegionLabelingEfficientGraph: public FilterMat2Mat<size_t>{

	public:
		RegionLabelingEfficientGraph(float sigma = 0.5, float k = 500, int min_size=30);
		virtual ~RegionLabelingEfficientGraph();
		size_t compute(const cv::Mat& col_image, const cv::Mat& mask,cv::Mat& segment_labels);
		inline size_t compute(const cv::Mat& col_image, cv::Mat& segment_labels){
			return compute(col_image,cv::Mat(),segment_labels);
		};
		
		float _sigma;//>get,set
		float _k;//>get,set
		int _min_size;//>get,set
	protected:
	private:
		
};

} // skl

#endif // __SKL_REGION_LABELING_EFFICIENT_GRAPH_H__

