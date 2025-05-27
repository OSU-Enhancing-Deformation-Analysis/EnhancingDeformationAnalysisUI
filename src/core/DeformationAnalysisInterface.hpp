#pragma once

#include <cstdint>
#include <functional>
#include <future>
#include <vector>

#include <utils.h>

#include <opencv2/opencv.hpp>

class DeformationAnalysisInterface {
      public:
	// Synchronous model execution
	static bool RunModel(std::vector<uint32_t *> &images, int width, int height, std::vector<Tile> &tiles,
			     const TileConfig &tile_config);

	// Asynchronous model execution with callback
	static std::future<bool> RunModelAsync(
	    std::vector<uint32_t *> &images, int width, int height, std::vector<Tile> &tiles,
	    const TileConfig &tile_config, std::function<void(bool)> callback = [](bool) {});

	static bool RunModelBatch(std::vector<uint32_t *> &images, int width, int height,
			    std::vector<Tile> &output_tiles, const TileConfig &tile_config,
			    const int batch_size = 1);

	static std::future<bool> RunModelBatchAsync(
	    std::vector<uint32_t *> &images, int width, int height, std::vector<Tile> &output_tiles,
	    const TileConfig &tile_config, const int batch_size = 1,
	    std::function<void(bool)> callback = [](bool) {});

	static bool IsProcessing() { return m_processing; }
	static float GetProgress() { return m_progress; }

      private:
	static bool m_processing;
	static float m_progress;
};
