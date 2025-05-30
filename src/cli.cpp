#include <algorithm>
#include <cli.hpp>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <core/CrackDetector.hpp>
#include <core/DenoiseInterface.hpp>
#include <core/FeatureTracker.hpp>
#include <core/ImageAnalysis.hpp>
#include <core/Stabilizer.hpp>
#include <utils.h>

static const char *USAGE = "usage: EnhancingDeformationAnalysisUI --folder <path> [--crop <pixels>] [--denoise "
			   "<filter> <tile> <overlap> <center> <includeOutside>]"
			   " [--analyze <stats.csv>] [--calculate-widths <widths.csv>] [--output <folder>]\n";

struct Settings {
	std::string folder;
	bool do_crop = false;
	int crop_pixels = 0;
	bool do_denoise = false;
	std::string filter;
	int denoise_tile = 256, denoise_ov = 0, denoise_ctr = 64;
	bool include_out = false;
	bool do_analyze = false;
	std::string stats_file;
	bool do_widths = false;
	std::string widths_file;
	std::string output;
};

void throwError(const std::string &flag, const std::string &msg, const char *prog) {
	throw std::runtime_error(std::format("msg: '{}'\n{}", flag, USAGE));
}

Settings parseArgs(int argc, char *argv[]) {
	Settings s;
	if (argc < 3)
		throwError("", "missing required arguments", argv[0]);
	std::vector<std::string> args(argv + 1, argv + argc);
	for (size_t i = 0; i < args.size(); ++i) {
		const auto &a = args[i];
		if (a == "--folder") {
			if (i + 1 >= args.size())
				throwError(a, "expected folder path", argv[0]);
			s.folder = args[++i];
		} else if (a == "--crop") {
			if (i + 1 >= args.size())
				throwError(a, "expected pixel count", argv[0]);
			s.crop_pixels = std::stoi(args[++i]);
			s.do_crop = true;
		} else if (a == "--denoise") {
			if (i + 5 >= args.size())
				throwError(a, "expected 5 args for denoise", argv[0]);
			s.filter = args[++i];
			s.denoise_tile = std::stoi(args[++i]);
			s.denoise_ov = std::stoi(args[++i]);
			s.denoise_ctr = std::stoi(args[++i]);
			s.include_out = std::stoi(args[++i]);
			s.do_denoise = true;
		} else if (a == "--analyze") {
			if (i + 1 >= args.size())
				throwError(a, "expected output CSV", argv[0]);
			s.stats_file = args[++i];
			s.do_analyze = true;
		} else if (a == "--calculate-widths") {
			if (i + 1 >= args.size())
				throwError(a, "expected widths CSV", argv[0]);
			s.widths_file = args[++i];
			s.do_widths = true;
		} else if (a == "--output") {
			if (i + 1 >= args.size())
				throwError(a, "expected output folder", argv[0]);
			s.output = args[++i];
		} else {
			throwError(a, "unknown flag", argv[0]);
		}
	}
	if (s.folder.empty())
		throwError("--folder", "folder is required", argv[0]);
	return s;
}

bool validate(const Settings &s) {
	if (s.do_denoise) {
		const std::vector<std::string> valid = {"blur",	     "sfr_hrsem",  "sfr_hrstem", "sfr_hrtem",
							"sfr_lrsem", "sfr_lrstem", "sfr_lrtem"};
		if (std::find(valid.begin(), valid.end(), s.filter) == valid.end()) {
			std::cerr << "invalid denoise filter: " << s.filter << "\n";
			return false;
		}
	}
	return true;
}

namespace cli {
void run(int argc, char *argv[]) {
	try {
		auto settings = parseArgs(argc, argv);
		if (!validate(settings))
			return;

		if (!std::filesystem::exists(settings.folder))
			throw std::runtime_error("folder not found: " + settings.folder);

		std::vector<uint32_t *> images;
		int w, h;
		if (!io::LoadTiffFolder(settings.folder.c_str(), images, w, h))
			throw std::runtime_error("failed to load images");

		if (settings.do_crop) {
			h -= settings.crop_pixels;
		}

		if (settings.do_denoise) {
			TileConfig cfg(TileType::Cropped, settings.denoise_tile, settings.denoise_ov,
				       settings.denoise_ctr, settings.include_out);
			if (settings.filter == "blur") {
				DenoiseInterface::Blur(images, w, h, 3, 1.0f);
			} else {
				DenoiseInterface::Denoise(images, w, h, settings.filter, cfg);
			}
		}

		if (settings.do_analyze) {
			std::vector<std::vector<float>> hists;
			std::vector<float> avg;
			std::vector<float> snrs;
			float avg_snr;
			ImageAnalysis::AnalyzeImages(images, w, h, hists, avg, snrs, avg_snr);
			io::SaveAnalysisCsv(settings.stats_file.c_str(), hists, avg, snrs, avg_snr);
		}

		if (settings.do_widths) {
			auto polys = CrackDetector::DetectCracks(images, w, h);
			auto widths = FeatureTracker::TrackCrackWidthProfiles(polys);
			io::WriteCSV(settings.widths_file.c_str(), widths);
		}

		if (!settings.output.empty()) {
			std::filesystem::create_directories(settings.output);
			for (size_t i = 0; i < images.size(); ++i) {
				std::string fn = settings.output + "/image_" + std::to_string(i) + ".tif";
				io::WriteTiff(fn.c_str(), images[i], w, h);
			}
		}

		if (!settings.do_crop && !settings.do_denoise && !settings.do_analyze && !settings.do_widths &&
		    settings.output.empty()) {
			std::cout << "nothing to do.\n";
		}
	} catch (const std::exception &e) {
		std::cerr << "error: " << e.what() << '\n';
		std::exit(1);
	}
}
} // namespace cli

// Load images from folder
bool loadImages(const Settings &settings, std::vector<uint32_t *> &images, int &width, int &height) {
	bool success = io::LoadTiffFolder(settings.folder.c_str(), images, width, height);
	if (!success) {
		printf("Failed to load images from %s\n", settings.folder.c_str());
		return false;
	}

	if (settings.do_crop) {
		if (settings.crop_pixels < 0 || settings.crop_pixels >= height) {
			printf("Invalid crop pixels: %d\n", settings.crop_pixels);
			printf("Crop pixels must be between 0 and %d\n", height - 1);
			return false;
		}
		height -= settings.crop_pixels;
	}

	return true;
}

// Apply denoising based on settings
void applyDenoising(const Settings &settings, std::vector<uint32_t *> &images, int width, int height) {
	TileConfig config = TileConfig(TileType::Cropped, settings.denoise_tile, settings.denoise_ov,
				       settings.denoise_ctr, settings.include_out);
	if (settings.do_denoise) {
		if (settings.filter == "blur") {
			DenoiseInterface::Blur(images, width, height, 3, 1.0f);
		} else {
			DenoiseInterface::Denoise(images, width, height, settings.filter, config);
		}
	}
}

// Perform image analysis based on settings
void performAnalysis(const Settings &settings, std::vector<uint32_t *> &images, int width, int height) {
	if (settings.do_analyze) {
		std::vector<std::vector<float>> histograms;
		std::vector<float> avg_histogram;
		std::vector<float> snrs;
		float avg_snr;

		ImageAnalysis::AnalyzeImages(images, width, height, histograms, avg_histogram, snrs, avg_snr);
		io::SaveAnalysisCsv(settings.stats_file.c_str(), histograms, avg_histogram, snrs, avg_snr);
	}
}

// Calculate crack widths based on settings
void calculateWidths(const Settings &settings, std::vector<uint32_t *> &images, int width, int height) {
	if (settings.do_widths) {
		auto polygons = CrackDetector::DetectCracks(images, width, height);
		auto widths = FeatureTracker::TrackCrackWidthProfiles(polygons);
		io::WriteCSV(settings.widths_file.c_str(), widths);
	}
}

// Save output images if output path provided
void saveOutputImages(const Settings &settings, std::vector<uint32_t *> &images, int width, int height) {
	if (!settings.output.empty()) {
		printf("Saving images to %s\n", settings.output.c_str());

		std::string outputPath = settings.output;
		// Remove trailing slash if present
		if (outputPath.back() == '/') {
			outputPath.pop_back();
		}

		if (!std::filesystem::exists(outputPath)) {
			std::filesystem::create_directory(outputPath);
		}

		for (int i = 0; i < images.size(); i++) {
			char filename[256];
			sprintf(filename, "%s/image_%d.tif", outputPath.c_str(), i);
			io::WriteTiff(filename, images[i], width, height);
		}
	}
}
