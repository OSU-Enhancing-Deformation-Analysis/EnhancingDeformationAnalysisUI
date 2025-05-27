#include <core/Stabilizer.hpp>

#include <core/ThreadPool.hpp>
#include <utils.h>

#include <opencv2/opencv.hpp>

// Static member initialization
float Stabilizer::m_progress = 0.0f;
bool Stabilizer::m_is_processing = false;

bool Stabilizer::Stabilize(std::vector<uint32_t *> &frames, int width,
			   int height) {
	PROFILE_FUNCTION();

	if (frames.empty())
		return false;

	std::vector<cv::Mat> mats;
	for (auto &ptr : frames) {
		cv::Mat img(height, width, CV_8UC4,
			    ptr); // Assuming RGBA format
		mats.push_back(
		    img.clone()); // Clone to avoid modifying original memory
	}

	std::vector<cv::Mat> stabilizedFrames;
	cv::Mat refGray, currGray, transformMatrix;

	cv::cvtColor(mats[0], refGray, cv::COLOR_RGBA2GRAY);
	stabilizedFrames.push_back(mats[0].clone());

	for (size_t i = 1; i < mats.size(); i++) {
		cv::cvtColor(mats[i], currGray, cv::COLOR_RGBA2GRAY);

		std::vector<cv::Point2f> refPts, currPts;
		std::vector<uchar> status;
		std::vector<float> err;

		cv::goodFeaturesToTrack(refGray, refPts, 200, 0.01, 30);
		if (refPts.empty()) {
			stabilizedFrames.push_back(mats[i].clone());
			continue; // Skip if no features found
		}

		cv::calcOpticalFlowPyrLK(refGray, currGray, refPts, currPts,
					 status, err);

		std::vector<cv::Point2f> filteredRef, filteredCurr;
		for (size_t j = 0; j < status.size(); j++) {
			if (status[j]) {
				filteredRef.push_back(refPts[j]);
				filteredCurr.push_back(currPts[j]);
			}
		}

		if (filteredRef.size() >= 4) {
			transformMatrix = cv::estimateAffinePartial2D(
			    filteredCurr,
			    filteredRef); // Reverse order to align to ref frame
			if (!transformMatrix.empty()) {
				transformMatrix.convertTo(
				    transformMatrix,
				    CV_64F); // Ensure correct format
			}
		}

		// Apply transformation
		cv::Mat stabilized;
		cv::warpAffine(mats[i], stabilized, transformMatrix,
			       mats[i].size(), cv::INTER_LINEAR,
			       cv::BORDER_CONSTANT);
		stabilizedFrames.push_back(stabilized);
		m_progress =
		    static_cast<float>(i) / mats.size(); // Update progress
	}

	for (size_t i = 0; i < frames.size(); i++) {
		std::memcpy(frames[i], stabilizedFrames[i].data,
			    width * height * 4);
	}
	return true;
}

std::future<bool>
Stabilizer::StabilizeAsync(std::vector<uint32_t *> &frames, int width,
			   int height, std::function<void(bool)> callback) {
	// Set processing flag
	m_is_processing = true;
	m_progress = 0.0f;
	// Get the thread pool
	auto &pool = ThreadPool::GetThreadPool();
	// Submit task to thread pool
	auto future = pool.enqueue([&frames, width, height, callback]() {
		bool result = Stabilize(frames, width, height);
		// When complete, update processing flag and call callback if
		// provided
		m_is_processing = false;
		if (callback) {
			callback(result);
		}
		return result;
	});
	m_progress = 1.0f; // Set progress to 100% immediately for async
	return future;
}
