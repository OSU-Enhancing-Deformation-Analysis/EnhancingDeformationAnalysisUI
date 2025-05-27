#pragma once

#include <string>
#include <vector>
#include <future>
#include <memory>

#include <ui/PreprocessingTab.h>

#include <OpenGL/Texture.h>

#include <core/FeatureTracker.hpp>
#include <core/DeformationAnalysisInterface.hpp>

#include <imgui.h>

class ImageSet {
      public:
	ImageSet(const std::string_view &folder_path);
	~ImageSet();

	void Display();

	bool Closed() { return !m_open; }

      private:
	void LoadImages();
	void DisplayImageComparisonTab();
	void DisplayImageAnalysisTab();
	void DisplayFeatureTrackingTab();
	void DisplayDeformationAnalysisTab();
	void DisplayTestTab();

	static int m_id_counter;

	bool m_open = true;
	std::string m_window_name;
	int m_window_id = 0;
	std::string m_folder_path;
	std::vector<std::shared_ptr<Texture>> m_textures;
	std::vector<std::shared_ptr<Texture>> m_processed_textures;
	uint32_t m_current_frame = 0;
	TileConfig m_tile_config = TileConfig();
	bool m_show_gallery_view = true; // For toggling between tile gallery and full image in deformation analysis

	PreprocessingTab m_preprocessing_tab;

	// feature tracking
	std::vector<cv::Point2f> m_points;
	std::vector<cv::Point2f> m_last_points;
	std::vector<std::vector<cv::Point2f>> m_last_tracked_points;
	uint32_t *m_point_image = nullptr;
	Texture m_point_texture;

	// image analysis
	uint32_t *m_ref_image = nullptr;
	uint32_t m_ref_image_width = 0, m_ref_image_height = 0;
	std::vector<std::vector<float>> histograms;
	std::vector<float> avg_histogram;
	std::vector<float> snrs;
	float avg_snr = 0.0f;
	bool write_success = true;
	int m_analysis_current_frame = 0;
	
	// region selection for analysis
	bool m_region_selection_active = false;
	bool m_region_selected = false;
	ImVec2 m_region_start = ImVec2(0, 0);
	ImVec2 m_region_end = ImVec2(0, 0);
	std::vector<float> m_region_histogram;
	float m_region_snr = 0.0f;

	// feature tracking
	std::vector<std::vector<std::vector<float>>> m_widths;
	std::vector<std::vector<float>> m_manual_widths;
	bool m_widths_write_success = true;
	std::chrono::time_point<std::chrono::system_clock> m_last_time =
	    std::chrono::system_clock::now();
	    
	// deformation analysis members
	std::vector<std::shared_ptr<Texture>> m_preview_tile_textures;
	std::vector<Tile> m_output_tiles;
	std::vector<std::shared_ptr<Texture>> m_output_tile_textures;
	std::shared_ptr<Texture> m_full_image_texture;
	std::vector<uint32_t*> m_processing_frames;
	bool m_model_ok = true;
	int m_batch_size = 8;
	uint32_t m_current_tile_index = 0;
	bool m_tile_need_refresh = false;
	
	// Async processing
	std::shared_ptr<std::future<bool>> m_processing_future;
};
