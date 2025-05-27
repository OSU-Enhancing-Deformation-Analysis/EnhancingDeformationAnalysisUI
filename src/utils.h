#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <functional>

#include <OpenGL/Texture.h>
#include <core/Tiler.hpp>

#include <opencv2/opencv.hpp>

namespace utils {
// TODO: fill in the filter for win32 (although it may not matter?)
std::string OpenFileDialog(const char *open_path = ".", const char *title = "",
			   const bool folders_only = false,
			   const char *filter = "");
std::string SaveFileDialog(const char *save_path = ".", const char *title = "",
			   const char *filter = "");

// gets the data from a texture
// assumes data is already allocated
void GetDataFromTexture(unsigned int *data, std::shared_ptr<Texture> texture,
			int width = 0, int height = 0);

// gets the data from a vector of textures
// doesn't assume data is already allocated and will allocate if not
void GetDataFromTextures(std::vector<uint32_t *> &data, int width, int height,
			 std::vector<std::shared_ptr<Texture>> &textures);

// loads data into textures and frees the data
// basically a helper function to consolidate a lot of the code in ImageSet.cpp
void LoadDataIntoTexturesAndFree(
    std::vector<std::shared_ptr<Texture>> &textures,
    std::vector<uint32_t *> &data, int width, int height);

// Creates textures from tiles for preview
void CreateTileTextures(
    std::vector<std::shared_ptr<Texture>> &tile_textures,
    const std::shared_ptr<Texture> &source_texture,
    const TileConfig &tile_config);
void UpdateTileTextures(
    std::vector<std::shared_ptr<Texture>> &tile_textures,
    const std::shared_ptr<Texture> &source_texture,
    const TileConfig &tile_config);

bool DirectoryContainsTiff(const std::filesystem::path &path);
} // namespace utils

// UI helper functions
namespace ui {
// Displays a window with tile previews
// Parameters:
//   window_title: The title of the window
//   is_open: Boolean reference that controls whether the window is open
//   tile_textures: Vector of texture pointers to display in the window
//   refresh_callback: Optional callback function to refresh the tiles
//   tile_size: Size of individual tile previews in the grid (default: 100)
//   columns: Number of tile columns in the grid (default: 4)
void DisplayTilePreviewWindow(
    const char* window_title,
    bool& is_open,
    std::vector<std::shared_ptr<Texture>>& tile_textures,
    std::function<void()> refresh_callback = nullptr,
    int tile_size = 100,
    int columns = 4);

// Displays a window for frame selection
// Parameters:
//   window_title: The title of the window
//   is_open: Boolean reference that controls whether the window is open
//   textures: Vector of texture pointers representing frames
//   selected_map: Map of selected indices
//   on_remove_callback: Callback function when removing selected frames
//   frame_size: Size of individual frame previews in the grid (default: 100)
//   columns: Number of frame columns in the grid (default: 6)
void DisplayFrameSelectionWindow(
    const char* window_title,
    bool& is_open,
    std::vector<std::shared_ptr<Texture>>& textures,
    std::map<int, int>& selected_map,
    std::function<void()> on_remove_callback,
    int frame_size = 100,
    int columns = 6);
} // namespace ui

namespace io {
uint32_t *LoadTiff(const char *path, int &width, int &height);

bool WriteTiff(const char *path, unsigned int *data, int width, int height);

bool LoadTiffFolder(const char *folder_path, std::vector<uint32_t *> &images,
		    int &width, int &height);

bool WriteGIFOfImageSet(const char *path,
			std::vector<std::shared_ptr<Texture>> images,
			int delay = 100, int loop = 0);

bool WriteCSV(const char *path,
	      std::vector<std::vector<std::vector<float>>> &data);
bool WriteCSV(const char *path, std::vector<std::vector<cv::Point2f>> &points,
	      std::vector<std::vector<float>> &data);

bool SaveAnalysisCsv(const char *path,
		     const std::vector<std::vector<float>> &histograms,
		     const std::vector<float> &avg_histogram,
		     const std::vector<float> &snrs, float avg_snr);
} // namespace io

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
