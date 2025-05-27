#include <core/Tiler.hpp>

std::vector<Tile> Tiler::CreateTiles(const cv::Mat &image, const TileConfig &config) {
	if (config.type == TileType::Cropped) {
		return CreateCroppedTiles(image, config);
	} else if (config.type == TileType::Blended) {
		return CreateBlendedTiles(image, config);
	}
	return {};
}

cv::Mat Tiler::StitchTiles(const std::vector<Tile> &tiles, const TileConfig &config, const cv::Size &originalSize) {
	if (config.type == TileType::Cropped) {
		return StitchCroppedTiles(tiles, originalSize, config);
	} else if (config.type == TileType::Blended) {
		return StitchBlendedTiles(tiles, originalSize, config);
	}
	return {};
}

std::vector<Tile> Tiler::CreateCroppedTiles(const cv::Mat &image, const TileConfig &config) {
	auto tileSize = config.tileSize;
	auto centerSize = config.centerSize;
	auto includeOutside = config.includeOutside;

	int height = image.rows;
	int width = image.cols;
	int centerInset = (tileSize - centerSize) / 2;
	std::vector<Tile> tiles;

	int y0 = includeOutside ? -centerInset : 0;
	for (int y = y0; y < height; y += centerSize) {
		for (int x = y0; x < width; x += centerSize) {
			int yStart = std::max(y, 0);
			int xStart = std::max(x, 0);
			int yEnd = std::min(y + tileSize, height);
			int xEnd = std::min(x + tileSize, width);

			int shiftY = yStart - y;
			int shiftX = xStart - x;

			cv::Mat roi = image(cv::Range(yStart, yEnd), cv::Range(xStart, xEnd));
			cv::Mat tile;
			if (roi.rows != tileSize || roi.cols != tileSize) {
				tile = cv::Mat::zeros(tileSize, tileSize, image.type());
				roi.copyTo(tile(cv::Rect(shiftX, shiftY, roi.cols, roi.rows)));
			} else {
				tile = roi.clone();
			}

			tiles.push_back({tile, cv::Point(x, y)});
		}
	}
	return tiles;
}

std::vector<Tile> Tiler::CreateBlendedTiles(const cv::Mat &image, const TileConfig &config) {
	auto tileSize = config.tileSize;
	auto overlap = config.overlap;

	std::vector<Tile> tiles;
	int h = image.rows, w = image.cols;
	for (int y = 0; y < h; y += tileSize - overlap) {
		for (int x = 0; x < w; x += tileSize - overlap) {
			int yEnd = std::min(y + tileSize, h);
			int xEnd = std::min(x + tileSize, w);
			cv::Rect roi(x, y, xEnd - x, yEnd - y);
			cv::Mat tile = image(roi);
			if (tile.rows != tileSize || tile.cols != tileSize) {
				cv::Mat padded = cv::Mat::zeros(tileSize, tileSize, image.type());
				tile.copyTo(padded(cv::Rect(0, 0, tile.cols, tile.rows)));
				tile = padded;
			}
			tiles.push_back({tile, cv::Point(x, y)});
		}
	}
	return tiles;
}

cv::Mat Tiler::StitchCroppedTiles(const std::vector<Tile> &tiles, const cv::Size &originalSize, const TileConfig &cfg) {
	if (tiles.empty())
		return cv::Mat();

	int ts = cfg.tileSize;
	int cs = cfg.centerSize;
	int inset = (ts - cs) / 2;
	int W = originalSize.width;
	int H = originalSize.height;

	cv::Mat result(H, W, CV_8UC4, cv::Scalar(0, 0, 0, 0));

	for (auto &tile : tiles) {
		int x = tile.position.x, y = tile.position.y;

		// dest rect
		int cx0 = std::max(x + inset, 0);
		int cy0 = std::max(y + inset, 0);
		int cx1 = std::min(x + ts - inset, W);
		int cy1 = std::min(y + ts - inset, H);

		// src rect in tile coords
		int tx0 = inset, ty0 = inset;
		int tx1 = cx1 - x, ty1 = cy1 - y;

		if (!cfg.includeOutside) {
			if (y < inset) {
				cy0 = 0;
				ty0 = 0;
			}
			if (y + ts > H) {
				cy1 = H;
				ty1 = std::min(ts, H - y);
				ty0 = 0;
			}
			if (x < inset) {
				cx0 = 0;
				tx0 = 0;
			}
			if (x + ts > W) {
				cx1 = W;
				tx1 = std::min(ts, W - x);
				tx0 = 0;
			}
		}

		int w = cx1 - cx0;
		int h = cy1 - cy0;
		if (w <= 0 || h <= 0)
			continue;

		tx0 = std::clamp(tx0, 0, ts - w);
		ty0 = std::clamp(ty0, 0, ts - h);

		cv::Rect srcR(tx0, ty0, w, h), dstR(cx0, cy0, w, h);
		cv::Mat src = tile.data(srcR);
		cv::Mat dst = result(dstR);

		if (src.type() == CV_32F) {
			// single-channel float → bgra
			cv::Mat u8, bgra;
			src.convertTo(u8, CV_8U, 255.0f);
			cv::cvtColor(u8, bgra, cv::COLOR_GRAY2BGRA);
			bgra.copyTo(dst);
		} else if (src.type() == CV_8UC4) {
			// already bgra
			src.copyTo(dst);
		}
		// else: skip unexpected types
	}

	return result;
}

cv::Mat Tiler::StitchBlendedTiles(const std::vector<Tile> &tiles, const cv::Size &originalSize, const TileConfig &cfg) {
	if (tiles.empty())
		return cv::Mat();

	int ts = cfg.tileSize;
	int ov = cfg.overlap;
	int W = originalSize.width;
	int H = originalSize.height;

	// determine channel count from first tile
	int ch = tiles[0].data.channels();

	// accumulators
	cv::Mat acc(H, W, CV_MAKETYPE(CV_32F, ch), cv::Scalar::all(0));
	cv::Mat weights(H, W, CV_32F, cv::Scalar::all(0));

	for (auto &tile : tiles) {
		int x = tile.position.x;
		int y = tile.position.y;
		int xEnd = std::min(x + ts, W);
		int yEnd = std::min(y + ts, H);
		cv::Rect dstR(x, y, xEnd - x, yEnd - y);
		cv::Rect srcR(0, 0, dstR.width, dstR.height);

		// convert to float (depth only)
		cv::Mat tf;
		if (tile.data.type() == CV_8UC4) {
			tile.data.convertTo(tf, CV_32F, 1.0 / 255.0);
		}
		else if (tile.data.type() == CV_32F) {
			tf = tile.data;
		}

		// build weight mask
		cv::Mat wm(dstR.height, dstR.width, CV_32F, 1.0f);
		if (ov > 0) {
			// horizontal ramps
			cv::Mat rampH(1, ov, CV_32F);
			for (int i = 0; i < ov; ++i)
				rampH.at<float>(0, i) = ov > 1 ? i / float(ov - 1) : 1.f;
			cv::Mat rampH_rev = 1 - rampH;

			// vertical ramps
			cv::Mat rampV(ov, 1, CV_32F);
			for (int i = 0; i < ov; ++i)
				rampV.at<float>(i, 0) = ov > 1 ? i / float(ov - 1) : 1.f;
			cv::Mat rampV_rev = 1 - rampV;

			if (x > 0)
				wm.colRange(0, ov).mul(cv::repeat(rampH, wm.rows, 1));
			if (xEnd < W)
				wm.colRange(wm.cols - ov, wm.cols).mul(cv::repeat(rampH_rev, wm.rows, 1));
			if (y > 0)
				wm.rowRange(0, ov).mul(cv::repeat(rampV.t(), 1, wm.cols));
			if (yEnd < H)
				wm.rowRange(wm.rows - ov, wm.rows).mul(cv::repeat(rampV_rev.t(), 1, wm.cols));

			// boost top/left borders
			if (x == 0) {
				cv::Mat r2 = 0.5f + 0.5f * rampH;
				wm.colRange(0, ov).mul(cv::repeat(r2, wm.rows, 1));
			}
			if (y == 0) {
				cv::Mat r2 = 0.5f + 0.5f * rampV;
				wm.rowRange(0, ov).mul(cv::repeat(r2.t(), 1, wm.cols));
			}
		}

		// replicate mask across channels if needed
		cv::Mat wmC;
		if (ch > 1) {
			std::vector<cv::Mat> v(ch, wm);
			cv::merge(v, wmC);
		} else {
			wmC = wm;
		}

		// blend & accumulate
		acc(dstR) += tf(srcR).mul(wmC);
		weights(dstR) += wm;
	}

	// avoid division by zero
	weights.setTo(1, weights == 0);

	// replicate weights per channel
	cv::Mat wC;
	if (ch > 1) {
		std::vector<cv::Mat> v(ch, weights);
		cv::merge(v, wC);
	} else {
		wC = weights;
	}

	// normalize
	cv::divide(acc, wC, acc);

	// after dividing acc by weights:
	cv::Mat u8;
	acc.convertTo(u8, CV_8U, 255.0f); // FLOAT → UINT8

	cv::Mat out(originalSize.height, originalSize.width, CV_8UC4);

	if (ch == 1) {
		cv::cvtColor(u8, out, cv::COLOR_GRAY2BGRA);
	} else if (ch == 4) {
		// u8 already has 4 channels in BGRA order
		out = u8;
	}

	return out;
}
