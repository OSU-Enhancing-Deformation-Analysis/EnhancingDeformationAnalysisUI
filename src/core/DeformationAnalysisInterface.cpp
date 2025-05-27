#include "DeformationAnalysisInterface.hpp"

#include <core/ThreadPool.hpp>

#include <torch/types.h>
#include <utils.h>

#include <opencv2/opencv.hpp>

#ifdef UI_INCLUDE_TENSORFLOW
#include <cppflow/cppflow.h>
#endif

#include <functional>
#include <future>
#include <torch/script.h>
#include <torch/torch.h>

bool DeformationAnalysisInterface::m_processing = false;
float DeformationAnalysisInterface::m_progress = 0.0f;

bool DeformationAnalysisInterface::RunModel(std::vector<uint32_t *> &images, int width, int height,
					    std::vector<Tile> &output_tiles, const TileConfig &tile_config) {
	PROFILE_FUNCTION();

	m_processing = true;
	m_progress = 0.0f;

#ifdef UI_INCLUDE_PYTORCH
	auto to_tensor = [](const cv::Mat &m) {
		return torch::from_blob(m.data, {1, 1, 256, 256}, torch::kUInt8).to(torch::kFloat32).clone();
	};

	auto dev = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU;
	// input: 2 images, each 256x256 and need to be converted to 1x256x256
	// (1 channel) input: data format: float between [0, 255] output: 2
	// images, each 1x256x256 (total output batchx2x256x256) output: data
	// format: float between [-2, 2]
	auto model = torch::jit::load("assets/models/batch-m4-combo.pt");
	model.to(dev);
	model.eval();

	// Make sure we have at least 2 images to process
	if (images.size() < 2) {
		m_progress = 1.0f;
		m_processing = false;
		return false;
	}

	for (size_t i = 0; i < images.size() - 1; ++i) {
		// Update progress
		m_progress = (float)i / std::max(1, (int)images.size() - 1);

		// === INPUT FORMATTING ===
		cv::Mat image = cv::Mat(height, width, CV_8UC4, images[i]);
		cv::cvtColor(image, image, cv::COLOR_BGRA2GRAY);
		auto tiles = Tiler::CreateTiles(image, tile_config);
		cv::Mat image2 = cv::Mat(height, width, CV_8UC4, images[i + 1]);
		cv::cvtColor(image2, image2, cv::COLOR_BGRA2GRAY);
		auto tiles2 = Tiler::CreateTiles(image2, tile_config);

		std::vector<Tile> outTiles;
		for (int k = 0; k < tiles.size(); ++k) {
			// === MODEL INFERENCE ===
			auto input = torch::cat({to_tensor(tiles[k].data), to_tensor(tiles2[k].data)}, 1).to(dev);

			auto out = model.forward({input}).toTensor().to(torch::kCPU); // out: torch::Tensor on cpu,
										      // shape 1,2,H,W

			// pull out three {H,W} uint8 planes
			auto t = out.squeeze(0);
			auto r = t.select(0, 0);
			auto g = t.select(0, 1);
			auto z = torch::zeros_like(r);

			// normalize & to uint8 in one go
			auto to_u8 = [&](torch::Tensor x) {
				return x
				    .add(2.0) // output is generally within [-2,
					      // 2] (it's odd)
				    .div(4.0)
				    .mul(255)
				    .clamp(0, 255)
				    .to(torch::kUInt8);
			};
			// two channels of actual info, one of zeros to complete
			// a 3-channel image
			auto ru8 = to_u8(r); // H,W
			auto gu8 = to_u8(g);
			auto zu8 = torch::zeros_like(ru8);

			// copy each into a cv::Mat and merge
			int H = ru8.size(0), W = ru8.size(1);
			cv::Mat mr(H, W, CV_8UC1, ru8.data_ptr<uint8_t>());
			cv::Mat mg(H, W, CV_8UC1, gu8.data_ptr<uint8_t>());
			cv::Mat mz(H, W, CV_8UC1, zu8.data_ptr<uint8_t>());

			std::vector<cv::Mat> chans = {mz, mg, mr};

			// merge the channels into one cv::Mat
			cv::Mat bgr;
			cv::merge(chans,
				  bgr); // b=0, g=chan1, r=chan0
					// convert to BGRA (add alpha channel)
			cv::cvtColor(bgr, bgr, cv::COLOR_BGR2BGRA);
			outTiles.push_back({bgr.clone(), tiles[k].position}); // own memory
			output_tiles.push_back(
			    {bgr.clone(), tiles[k].position, (int)i}); // own memory with source frame index
		}
		auto stitched = Tiler::StitchTiles(outTiles, tile_config, image.size());

		// copy back to the original image
		memcpy(images[i], stitched.data, width * height * sizeof(uint32_t));
	}

	m_progress = 1.0f;
	m_processing = false;
	return true;
#else  // UI_INCLUDE_PYTORCH
	m_processing = false;
	return false;
#endif // UI_INCLUDE_PYTORCH
}

// Asynchronous version of the model execution
std::future<bool> DeformationAnalysisInterface::RunModelAsync(std::vector<uint32_t *> &images, int width, int height,
							      std::vector<Tile> &tiles, const TileConfig &tile_config,
							      std::function<void(bool)> callback) {

	m_processing = true;
	m_progress = 0.0f;

	// Creates a task that will be run asynchronously - we capture images by value (as a copy)
	auto task = [=]() mutable {
		bool result = RunModel(images, width, height, tiles, tile_config);
		callback(result);
		return result;
	};

	return std::async(std::launch::async, task);
}

bool DeformationAnalysisInterface::RunModelBatch(std::vector<uint32_t *> &images, int width, int height,
						 std::vector<Tile> &output_tiles, const TileConfig &tile_config,
						 const int batch_size) // ← new adjustable batch size
{
	PROFILE_FUNCTION();

	m_processing = true;
	m_progress = 0.0f;

#ifdef UI_INCLUDE_PYTORCH
	auto to_tensor = [](const cv::Mat &m) {
		return torch::from_blob(m.data, {1, 1, 256, 256}, torch::kUInt8).to(torch::kFloat32).clone();
	};
	auto to_u8 = [&](torch::Tensor x) { return x.add(2.0).div(4.0).mul(255).clamp(0, 255).to(torch::kUInt8); };

	auto dev = torch::cuda::is_available() ? torch::kCUDA : torch::kCPU;
	auto model = torch::jit::load("assets/models/batch-m4-combo.pt");
	model.to(dev);
	model.eval();

	if (images.size() < 2) {
		m_progress = 1.0f;
		m_processing = false;
		return false;
	}

	for (size_t i = 0; i < images.size() - 1; ++i) {
		PROFILE_SCOPE(DeformationOneFrame);

		m_progress = float(i) / float(images.size() - 1);

		// prepare grayscale tiles for frame i and i+1
		cv::Mat img1(height, width, CV_8UC4, images[i]);
		cv::cvtColor(img1, img1, cv::COLOR_BGRA2GRAY);
		auto tiles1 = Tiler::CreateTiles(img1, tile_config);

		cv::Mat img2(height, width, CV_8UC4, images[i + 1]);
		cv::cvtColor(img2, img2, cv::COLOR_BGRA2GRAY);
		auto tiles2 = Tiler::CreateTiles(img2, tile_config);

		std::vector<Tile> outTiles;
		size_t total = tiles1.size();

		for (size_t k = 0; k < total; k += batch_size) {
			PROFILE_SCOPE(BatchProcessing);

			size_t curr_batch = std::min((size_t)batch_size, total - k);

			// build batch of tensors
			std::vector<torch::Tensor> batch_inputs;
			batch_inputs.reserve(curr_batch);
			for (size_t j = 0; j < curr_batch; ++j) {
				auto t0 = to_tensor(tiles1[k + j].data);
				auto t1 = to_tensor(tiles2[k + j].data);
				batch_inputs.push_back(torch::cat({t0, t1}, 1));
			}

			auto raw = torch::stack(batch_inputs, 0);
			auto squeezed = raw.squeeze(1);			  // shape (B,2,H,W)
			auto input_batch = squeezed.contiguous().to(dev); // shape (B,2,H,W)

			// single forward for the whole batch
			auto out_batch = model.forward({input_batch}).toTensor().to(torch::kCPU); // shape (B,2,H,W)

			// unpack each element
			for (size_t j = 0; j < curr_batch; ++j) {
				PROFILE_SCOPE(UnpackBatch);

				auto t = out_batch[j];
				auto r = t.select(0, 0), g = t.select(0, 1);

				// convert channels to uint8 mats
				auto ru8 = to_u8(r), gu8 = to_u8(g);
				auto zu8 = torch::zeros_like(ru8);

				int H = (int)ru8.size(0), W = (int)ru8.size(1);
				cv::Mat mr(H, W, CV_8UC1, ru8.data_ptr<uint8_t>());
				cv::Mat mg(H, W, CV_8UC1, gu8.data_ptr<uint8_t>());
				cv::Mat mz(H, W, CV_8UC1, zu8.data_ptr<uint8_t>());

				std::vector<cv::Mat> chans = {mz, mg, mr};
				cv::Mat bgr;
				cv::merge(chans, bgr);
				cv::cvtColor(bgr, bgr, cv::COLOR_BGR2BGRA);

				outTiles.push_back({bgr.clone(), tiles1[k + j].position});
				output_tiles.push_back({bgr.clone(), tiles1[k + j].position, (int)i});
			}
		}

		auto stitched = Tiler::StitchTiles(outTiles, tile_config, img1.size());
		memcpy(images[i], stitched.data, width * height * sizeof(uint32_t));
	}

	m_progress = 1.0f;
	m_processing = false;
	return true;
#else
	m_processing = false;
	return false;
#endif
}

std::future<bool> DeformationAnalysisInterface::RunModelBatchAsync(std::vector<uint32_t *> &images, int width,
								   int height, std::vector<Tile> &output_tiles,
								   const TileConfig &tile_config, const int batch_size,
								   std::function<void(bool)> callback) {
	// Set processing flag
	m_processing = true;
	m_progress = 0.0f;

	// Get the thread pool
	auto &pool = ThreadPool::GetThreadPool();

	// Submit task to thread pool
	auto future = pool.enqueue([&images, width, height, &output_tiles, tile_config, batch_size, callback]() {
		bool result = RunModelBatch(images, width, height, output_tiles, tile_config, batch_size);

		// When complete, update processing flag and call callback
		// if provided
		m_processing = false;
		if (callback) {
			callback(result);
		}

		return result;
	});

	return future;
}
