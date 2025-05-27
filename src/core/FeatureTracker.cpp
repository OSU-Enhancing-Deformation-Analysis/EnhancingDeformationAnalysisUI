#include <core/FeatureTracker.hpp>

#include <opencv2/opencv.hpp>

#include <utils.h>

std::vector<std::vector<float>> FeatureTracker::TrackFeatures(
    const std::vector<uint32_t *> &images, std::vector<cv::Point2f> &points,
    std::vector<std::vector<cv::Point2f>> &trackedPoints, int width,
    int height) {
	PROFILE_FUNCTION();

	if (images.empty() || points.empty())
		return {};
	std::vector<std::vector<float>> widths;
	cv::Mat prevGray, currGray;
	std::vector<cv::Point2f> prevPts, currPts;
	cv::TermCriteria criteria(
	    cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 50, 0.001);
	trackedPoints.clear();

	// Convert first image from uint32_t* to cv::Mat (assuming BGRA format)
	cv::Mat firstImage(height, width, CV_8UC4, images[0]);
	cv::Mat displayImage = firstImage.clone();
	cv::cvtColor(firstImage, prevGray, cv::COLOR_BGRA2GRAY);

	// Refine each point separately with its own mask
	int radius = 5;		       // Adjust radius as needed
	prevPts.resize(points.size()); // Pre-allocate for points
	for (int i = 0; i < points.size(); i++) {
		// Create a mask for this point only
		cv::Mat mask = cv::Mat::zeros(height, width, CV_8U);
		cv::circle(mask, points[i], radius, 255,
			   -1); // Mask around this point

		// Find one good feature in this region
		std::vector<cv::Point2f> refinedPts;
		cv::goodFeaturesToTrack(prevGray, refinedPts, 1, 0.01, 10, mask,
					3, false, 0.04); // maxCorners = 1
		if (refinedPts.empty()) {
			prevPts[i] = points[i]; // Fallback to user point if no
						// feature found
		} else {
			prevPts[i] =
			    refinedPts[0]; // Use the single refined point
		}
	}
	trackedPoints.push_back(prevPts);

	widths.resize(images.size());
	// Draw initial points and distance on first image
	for (int i = 0; i < points.size(); i++) {
		if (i % 2 == 0)
			widths[0].push_back(
			    cv::norm(prevPts[i] - prevPts[i + 1]));
		cv::circle(displayImage, prevPts[i], 5,
			   cv::Scalar(0, 255, 0, 255), -1);
	}
	memcpy(images[0], displayImage.data,
	       width * height * 4); // Write back to original buffer

	// Track through sequence
	for (size_t i = 1; i < images.size(); i++) {
		cv::Mat currentImage(height, width, CV_8UC4, images[i]);
		cv::cvtColor(currentImage, currGray, cv::COLOR_BGRA2GRAY);

		std::vector<uchar> status;
		std::vector<float> err;
		cv::calcOpticalFlowPyrLK(prevGray, currGray, prevPts, currPts,
					 status, err, cv::Size(21, 21), 3,
					 criteria);

		std::vector<cv::Point2f> framePoints;
		for (int j = 0; j < points.size(); j++) {
			if (status[j])
				framePoints.push_back(currPts[j]);
			else
				framePoints.push_back(cv::Point2f(-1, -1));
			if (j % 2 == 0)
				widths[i].push_back(
				    cv::norm(currPts[j] - currPts[j + 1]));
			cv::circle(currentImage, currPts[j], 5,
				   cv::Scalar(0, 255, 0, 255), -1);
		}
		trackedPoints.push_back(framePoints);
		memcpy(images[i], currentImage.data,
		       width * height * 4); // Write back to original buffer

		cv::swap(prevGray, currGray);
		prevPts = currPts;
	}
	return widths;
}

std::vector<float> FeatureTracker::CalculateCrackWidthProfile(
    const std::vector<cv::Point> &polygon) {
	std::vector<float> widths;
	if (polygon.size() < 3)
		return widths;

	int n = polygon.size();
	int sampleCount = std::max(5, n); // Sample ~20% of points, min 5
	for (int i = 0; i < n; i += std::max(1, n / sampleCount)) {
		cv::Point p1 = polygon[i];
		std::vector<float> distances;

		// Compute distances to all other points
		for (const auto &p2 : polygon) {
			float dist = cv::norm(p2 - p1);
			if (dist > 0)
				distances.push_back(dist);
		}

		// Take second smallest distance as width at this point
		if (distances.size() > 1) {
			std::sort(distances.begin(), distances.end());
			widths.push_back(distances[1]);
		}
	}

	return widths;
}

// Track width profiles for all cracks in all images
std::vector<std::vector<std::vector<float>>>
FeatureTracker::TrackCrackWidthProfiles(
    const std::vector<std::vector<std::vector<cv::Point>>> &polygons) {
	PROFILE_FUNCTION();

	std::vector<std::vector<std::vector<float>>> profilesPerImage;

	for (const auto &imagePolygons : polygons) { // For each image
		std::vector<std::vector<float>> profiles;
		for (const auto &polygon : imagePolygons) { // For each crack
			profiles.push_back(CalculateCrackWidthProfile(polygon));
		}
		profilesPerImage.push_back(profiles);
	}

	return profilesPerImage;
}
