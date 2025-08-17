#pragma once

#include <cstdint>
#include <vector>

class ImageAnalysis {
      public:
	static void AnalyzeImages(std::vector<uint32_t *> &frames, int width,
				  int height,
				  std::vector<std::vector<float>> &histograms,
				  std::vector<float> &avg_histogram,
				  std::vector<float> &snrs, float &avg_snr);
	
	// Regional analysis for a specific area of an image
	static void AnalyzeRegion(uint32_t *frame, int width, int height,
				  int roi_x, int roi_y, int roi_width, int roi_height,
				  std::vector<float> &histogram, float &snr);
};
