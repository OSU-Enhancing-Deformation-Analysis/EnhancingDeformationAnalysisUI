#pragma once

#include <OpenGL/Texture.h>

#include <opencv2/opencv.hpp>

struct Tile {
	cv::Mat data;
	cv::Point position;
	int sourceFrameIndex = -1; // Stores the index of the source frame
};

enum class TileType { Cropped, Blended };

struct TileConfig {
	TileConfig(TileType type = TileType::Cropped, int tileSize = 256, int overlap = 0, int centerSize = 64,
		   bool includeOutside = false)
	    : type(type), tileSize(tileSize), overlap(overlap), centerSize(centerSize), includeOutside(includeOutside) {
	}

	bool operator==(const TileConfig &other) const {
		return type == other.type && tileSize == other.tileSize && overlap == other.overlap &&
		       centerSize == other.centerSize && includeOutside == other.includeOutside;
	}

	TileType type;
	int tileSize;
	int overlap;
	int centerSize;
	bool includeOutside;
};

class Tiler {
      public:
	static std::vector<Tile> CreateTiles(const cv::Mat &image, const TileConfig &config);
	static cv::Mat StitchTiles(const std::vector<Tile> &tiles, const TileConfig &config,
				   const cv::Size &originalSize);

      private:
	static std::vector<Tile> CreateCroppedTiles(const cv::Mat &image, const TileConfig &config);
	static std::vector<Tile> CreateBlendedTiles(const cv::Mat &image, const TileConfig &config);
	static cv::Mat StitchCroppedTiles(const std::vector<Tile> &tiles, const cv::Size &originalSize,
					  const TileConfig &config);
	static cv::Mat StitchBlendedTiles(const std::vector<Tile> &tiles, const cv::Size &originalSize,
					  const TileConfig &config);
};
