#pragma once

#include <vector>
#include <future>
#include <memory>

#include <utils.h>

#include <OpenGL/Texture.h>
#include <core/DenoiseInterface.hpp>
#include <core/Stabilizer.hpp>
#include <core/CrackDetector.hpp>

class PreprocessingTab {
	public:
		PreprocessingTab() = default;
		PreprocessingTab(std::vector<std::shared_ptr<Texture>>& textures, std::vector<std::shared_ptr<Texture>>& processed_textures);
		// don't let this free the textures/texture vectors, they are allocated and freed in ImageSet
		~PreprocessingTab() {}
		void DisplayPreprocessingTab(bool& changed);
		void GetProcessedTextures(std::vector<std::shared_ptr<Texture>>& processed_textures) { processed_textures = m_processed_textures; }
		void SetProcessedTextures(std::vector<std::shared_ptr<Texture>>& processed_textures) { m_processed_textures = processed_textures; }

		// Check if processing is currently happening
		bool IsProcessing() const { return m_is_processing; }

		// Get the current progress (0.0 to 1.0)
		float GetProgress() const {
			// Processing status display
			float progress = 0.0f;
			if (DenoiseInterface::IsProcessing()) {
				progress = DenoiseInterface::GetProgress();
			} else if (Stabilizer::IsProcessing()) {
				progress = Stabilizer::GetProgress();
			}
			else if (CrackDetector::IsProcessing()) {
				progress = CrackDetector::GetProgress();
			}
			return progress;
		}

	private:
		// Helper methods to update UI after async processing completes
		void OnProcessingComplete(bool success);
		
		// Store which frames were processed for restoration
		std::vector<int> m_processed_frame_indices;
		
		// Helper method to get frames to process based on selection
		std::vector<int> GetFramesToProcess() const;

		// Our textures
		std::vector<std::shared_ptr<Texture>> m_textures;
		std::vector<std::shared_ptr<Texture>> m_processed_textures;
		std::map<int, int> m_selected_textures_map;

		// for splitting tiles and denoising
		std::vector<std::shared_ptr<Texture>> m_split_textures;
		int m_tile_size = 256; 
		int m_center_size = 64;
		int m_overlap = 0;
		bool m_include_outside = false;
		TileConfig m_tile_config = TileConfig(TileType::Cropped, m_tile_size, m_overlap, m_center_size, m_include_outside);
		bool m_tile_need_refresh = false;

		static const char* m_models[];
		int m_selected_model = 0;

		std::vector<uint32_t*> m_processing_frames;
		std::shared_ptr<std::future<bool>> m_processing_future;
		bool m_is_processing = false;
		bool m_last_result = true;

		int m_kernel_size = 3;
		float m_sigma = 1.0f;

		// Crack detection parameters
		int m_crack_darkness = 40;
		int m_fill_threshold = 2;
		int m_sharpness = 50;
		int m_resolution = 3;
		int m_amount = 1;
		
		// Frame navigation
		int m_current_frame = 0;
		
		// Processing mode
		bool m_process_selected_only = false;
};
