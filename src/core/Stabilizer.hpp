#pragma once

#include <cstdint>
#include <functional>
#include <future>
#include <vector>

class Stabilizer {
      public:
	static bool Stabilize(std::vector<uint32_t *> &frames, int width,
			      int height);
	static std::future<bool>
	StabilizeAsync(std::vector<uint32_t *> &frames, int width, int height,
		       std::function<void(bool)> callback = nullptr);

	static bool IsProcessing() { return m_is_processing; }
	static float GetProgress() { return m_progress; }

      private:
	static float m_progress;
	static bool m_is_processing;
};
