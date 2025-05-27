#pragma once

#include <cstdint>
#include <vector>

#include <opencv2/opencv.hpp>

class FeatureTracker {
      public:
	static std::vector<std::vector<float>>
	TrackFeatures(const std::vector<uint32_t *> &imageSequence,
		      std::vector<cv::Point2f> &points,
		      std::vector<std::vector<cv::Point2f>> &trackedPoints,
		      int width, int height);
	static std::vector<float>
	CalculateCrackWidthProfile(const std::vector<cv::Point> &polygon);
	static std::vector<std::vector<std::vector<float>>>
	TrackCrackWidthProfiles(
	    const std::vector<std::vector<std::vector<cv::Point>>> &polygons);
};
