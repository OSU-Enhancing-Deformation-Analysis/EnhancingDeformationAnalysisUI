#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include <deformation_core/Tiler.hpp>

#include <opencv2/opencv.hpp>

namespace utils {

bool DirectoryContainsTiff(const std::filesystem::path &path);
} // namespace utils

// IO functions for loading and saving images, TIFFs, GIFs, and CSVs
namespace io {
uint32_t *LoadTiff(const char *path, int &width, int &height);

bool WriteTiff(const char *path, unsigned int *data, int width, int height);

bool LoadTiffFolder(const char *folder_path, std::vector<uint32_t *> &images, int &width, int &height);

bool WriteCSV(const char *path, std::vector<std::vector<std::vector<float>>> &data);
bool WriteCSV(const char *path, std::vector<std::vector<cv::Point2f>> &points, std::vector<std::vector<float>> &data);

bool SaveAnalysisCsv(const char *path, const std::vector<std::vector<float>> &histograms,
		     const std::vector<float> &avg_histogram, const std::vector<float> &snrs, float avg_snr);
} // namespace io

// Profiler class for measuring performance of code sections
class Profiler {
      public:
	Profiler(const char *name = "'empty'");
	~Profiler();
	void Stop();

      private:
	const char *name;
	std::chrono::high_resolution_clock::time_point start_time;
	bool stopped = false;
};

#ifdef UI_PROFILE
#define PROFILE_FUNCTION(name) Profiler profiler_##name(__FUNCTION__);
#define PROFILE_SCOPE(name) Profiler profiler_##name(#name);
#else
#define PROFILE_FUNCTION(name)
#define PROFILE_SCOPE(name)
#endif
