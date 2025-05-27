#pragma once

#include <cstdint>
#include <functional>
#include <future>
#include <string>
#include <vector>

#include <OpenGL/Texture.h>
#include <core/Tiler.hpp>

class DenoiseInterface {
      public:
	// Synchronous methods
	static bool Denoise(std::vector<uint32_t *> &images, int width,
			    int height, const std::string &model_name,
			    const TileConfig &config);
	static bool Blur(std::vector<uint32_t *> &images, int width, int height,
			 int kernel_size, float sigma);

	// Asynchronous methods with callback
	static std::future<bool>
	DenoiseAsync(std::vector<uint32_t *> &images, int width, int height,
		     const std::string &model_name, const TileConfig &config,
		     std::function<void(bool)> callback = nullptr);
	static std::future<bool>
	BlurAsync(std::vector<uint32_t *> &images, int width, int height,
		  int kernel_size, float sigma,
		  std::function<void(bool)> callback = nullptr);

	// Status checking
	static bool IsProcessing() { return m_is_processing; }
	static float GetProgress() { return m_progress; }

      private:
	static float m_progress;
	static bool m_is_processing;
};
