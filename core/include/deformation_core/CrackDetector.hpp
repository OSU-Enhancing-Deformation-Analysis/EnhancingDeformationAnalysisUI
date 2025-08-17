#pragma once

#include <cstdint>
#include <functional>
#include <future>
#include <vector>

#include <opencv2/opencv.hpp>

class CrackDetector {
      public:
	static std::vector<std::vector<std::vector<cv::Point>>>
	DetectCracks(const std::vector<uint32_t *> &images, int width,
		     int height, int crack_darkness = 40,
		     int fill_threshold = 2, int sharpness = 50,
		     int resolution = 3, int amount = 1);
	static std::future<bool>
	DetectCracksAsync(const std::vector<uint32_t *> &images, int width,
			  int height, int crack_darkness = 40,
			  int fill_threshold = 2, int sharpness = 50,
			  int resolution = 3, int amount = 1,
			  std::function<void(bool)> callback = nullptr);
	static std::future<std::vector<std::vector<std::vector<cv::Point>>>>
	DetectCracksDataAsync(const std::vector<uint32_t *> &images, int width,
			      int height, int crack_darkness = 40,
			      int fill_threshold = 2, int sharpness = 50,
			      int resolution = 3, int amount = 1);

	static bool IsProcessing() { return m_is_processing; }
	static float GetProgress() { return m_progress; }

      private:
	static bool m_is_processing;
	static float m_progress;
};
