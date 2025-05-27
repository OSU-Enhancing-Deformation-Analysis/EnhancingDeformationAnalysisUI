#include <cli.hpp>

#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <stdio.h>

#include <core/CrackDetector.hpp>
#include <core/DenoiseInterface.hpp>
#include <core/FeatureTracker.hpp>
#include <core/ImageAnalysis.hpp>
#include <core/Stabilizer.hpp>

#include <utils.h>

// Settings struct
struct Settings {
	std::string folder;
	int crop_pixels = 0;
	bool do_crop = false;
	std::string filter;
	int denoise_tile_size = 256;
	int denoise_overlap = 0;
	int denoise_center_size = 64;
	bool includeOutside = false;
	bool do_denoise = false;
	std::string stats_output;
	bool do_analyze = false;
	std::string widths_output;
	bool do_widths = false;
	std::string output;
};

namespace cli {

// Helper to check if a string is a flag
bool isFlag(const std::string &arg) {
	return arg.size() >= 2 && arg[0] == '-' && arg[1] == '-';
}

// Helper to print usage error and exit
void printUsageError(const char *flag, const char *msg, const char *prog_name) {
	std::cerr << std::string(msg) + " '" + flag + "'\n" +
			 "Usage: " + prog_name +
			 " --folder <path> [--crop <pixels>] [--denoise "
			 "<blur/sfr_hrsem/sfr_lrsem>] [--analyze <output.csv>] "
			 "[--calculate-widths <widths.csv>] [--output "
			 "<folder_path>]\n";
	exit(1);
}

// Helper to print usage information
void printUsage(const char *prog_name) {
	std::cout << "Usage: " << prog_name
		  << " --folder <path> [--crop <pixels>] [--denoise <blur/...> "
		     "<tile_size>] [--analyze <output.csv>] "
		  << "[--calculate-widths <widths.csv>] [--output <path>]\n";
}

// Parse command line arguments
Settings parseArguments(int argc, char *argv[]) {
	Settings settings;

	// Map of flag to {handler, expected_arg_count}
	// arg_count: 0 for no args, 1 for one arg
	std::map<std::string,
		 std::pair<std::function<void(int &, int, char *[])>, int>>
	    handlers = {
		{"--folder",
		 {{[&](int &i, int argc, char *argv[]) {
			  if (i + 1 >= argc)
				  printUsageError("--folder",
						  "Missing folder path",
						  argv[0]);
			  settings.folder = argv[++i];
			  if (settings.folder.empty())
				  printUsageError("--folder",
						  "Folder path cannot be empty",
						  argv[0]);
		  }},
		  1}},
		{"--crop",
		 {{[&](int &i, int argc, char *argv[]) {
			  if (i + 1 >= argc)
				  printUsageError(
				      "--crop", "Missing pixel count", argv[0]);
			  try {
				  settings.crop_pixels = std::stoi(argv[++i]);
				  if (settings.crop_pixels < 0)
					  printUsageError("--crop",
							  "Pixel count must be "
							  "non-negative",
							  argv[0]);
				  settings.do_crop = true;
			  } catch (const std::exception &e) {
				  printUsageError(
				      "--crop", "Invalid pixel count", argv[0]);
			  }
		  }},
		  1}},
		{"--denoise",
		 {{[&](int &i, int argc, char *argv[]) {
			  if (i + 1 >= argc)
				  printUsageError("--denoise",
						  "Missing filter type",
						  argv[0]);
			  settings.filter = argv[++i];
			  if (settings.filter.empty())
				  printUsageError("--denoise",
						  "Filter type cannot be empty",
						  argv[0]);
			  if (i + 1 >= argc)
				  printUsageError("--denoise",
						  "Missing tile size", argv[0]);
			  settings.denoise_tile_size = std::stoi(argv[++i]);
			  if (i + 1 >= argc)
				  printUsageError("--denoise",
						  "Missing overlap size",
						  argv[0]);
			  settings.denoise_overlap = std::stoi(argv[++i]);
			  if (i + 1 >= argc)
				  printUsageError("--denoise",
						  "Missing center size",
						  argv[0]);
			  settings.denoise_center_size = std::stoi(argv[++i]);
			  if (i + 1 >= argc)
				  printUsageError("--denoise",
						  "Missing includeOutside flag",
						  argv[0]);
			  settings.includeOutside = std::stoi(argv[++i]);

			  settings.do_denoise = true;
		  }},
		  5}},
		{"--analyze",
		 {{[&](int &i, int argc, char *argv[]) {
			  if (i + 1 >= argc)
				  printUsageError("--analyze",
						  "Missing output file",
						  argv[0]);
			  settings.stats_output = argv[++i];
			  if (settings.stats_output.empty())
				  printUsageError("--analyze",
						  "Output file cannot be empty",
						  argv[0]);
			  settings.do_analyze = true;
		  }},
		  1}},
		{"--calculate-widths",
		 {{[&](int &i, int argc, char *argv[]) {
			  if (i + 1 >= argc)
				  printUsageError("--calculate-widths",
						  "Missing output file",
						  argv[0]);
			  settings.widths_output = argv[++i];
			  if (settings.widths_output.empty())
				  printUsageError("--calculate-widths",
						  "Output file cannot be empty",
						  argv[0]);
			  settings.do_widths = true;
		  }},
		  1}},
		{"--output",
		 {{[&](int &i, int argc, char *argv[]) {
			  if (i + 1 >= argc)
				  printUsageError("--output",
						  "Missing output path",
						  argv[0]);
			  settings.output = argv[++i];
			  if (settings.output.empty())
				  printUsageError("--output",
						  "Output path cannot be empty",
						  argv[0]);
		  }},
		  1}},
		{"--help",
		 {{[&](int &i, int argc, char *argv[]) {
			  printUsage(argv[0]);
			  exit(0);
		  }},
		  0}}};

	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		auto it = handlers.find(arg);
		if (it == handlers.end()) {
			printUsageError(arg.c_str(), "Unknown flag", argv[0]);
		}

		auto &[handler, expected_args] = it->second;
		int start_i = i;

		// Execute the handler
		handler(i, argc, argv);

		// Calculate actual args consumed (excluding the flag itself)
		int args_consumed = i - start_i;

		// Check that consumed args aren't flags (except the initial
		// flag)
		for (int j = start_i + 1; j <= i; ++j) {
			if (isFlag(argv[j])) {
				printUsageError(
				    arg.c_str(),
				    "Argument cannot be another flag", argv[0]);
			}
		}
	}

	if (settings.folder.empty()) {
		printUsageError("--folder", "Required flag missing", argv[0]);
	}

	return settings;
}

// Validate settings after parsing
bool validateSettings(const Settings &settings, char **argv) {
	if (settings.folder.empty()) {
		printf("Error: --folder is required\n");
		printUsage(argv[0]);
		return false;
	}

	// Validate denoise filter
	if (settings.do_denoise) {
		const char *validModels[] = {
		    "blur",	 "sfr_hrsem",  "sfr_hrstem", "sfr_hrtem",
		    "sfr_lrsem", "sfr_lrstem", "sfr_lrtem"};
		bool found = false;

		for (int i = 0; i < 7; i++) {
			if (settings.filter == validModels[i]) {
				found = true;
				break;
			}
		}

		if (!found) {
			printf("Invalid denoise filter: %s\n",
			       settings.filter.c_str());
			printf("These are the available filters: blur, "
			       "sfr_hrsem, sfr_hrstem, "
			       "sfr_hrtem, sfr_lrsem, sfr_lrstem, sfr_lrtem\n");
			return false;
		}
	}

	return true;
}

// Load images from folder
bool loadImages(const Settings &settings, std::vector<uint32_t *> &images,
		int &width, int &height) {
	bool success =
	    io::LoadTiffFolder(settings.folder.c_str(), images, width, height);
	if (!success) {
		printf("Failed to load images from %s\n",
		       settings.folder.c_str());
		return false;
	}

	if (settings.do_crop) {
		if (settings.crop_pixels < 0 ||
		    settings.crop_pixels >= height) {
			printf("Invalid crop pixels: %d\n",
			       settings.crop_pixels);
			printf("Crop pixels must be between 0 and %d\n",
			       height - 1);
			return false;
		}
		height -= settings.crop_pixels;
	}

	return true;
}

// Apply denoising based on settings
void applyDenoising(const Settings &settings, std::vector<uint32_t *> &images,
		    int width, int height) {
	TileConfig config =
	    TileConfig(TileType::Cropped, settings.denoise_tile_size,
		       settings.denoise_overlap, settings.denoise_center_size,
		       settings.includeOutside);
	if (settings.do_denoise) {
		if (settings.filter == "blur") {
			DenoiseInterface::Blur(images, width, height, 3, 1.0f);
		} else {
			DenoiseInterface::Denoise(images, width, height,
						  settings.filter, config);
		}
	}
}

// Perform image analysis based on settings
void performAnalysis(const Settings &settings, std::vector<uint32_t *> &images,
		     int width, int height) {
	if (settings.do_analyze) {
		std::vector<std::vector<float>> histograms;
		std::vector<float> avg_histogram;
		std::vector<float> snrs;
		float avg_snr;

		ImageAnalysis::AnalyzeImages(images, width, height, histograms,
					     avg_histogram, snrs, avg_snr);
		io::SaveAnalysisCsv(settings.stats_output.c_str(), histograms,
				    avg_histogram, snrs, avg_snr);
	}
}

// Calculate crack widths based on settings
void calculateWidths(const Settings &settings, std::vector<uint32_t *> &images,
		     int width, int height) {
	if (settings.do_widths) {
		auto polygons =
		    CrackDetector::DetectCracks(images, width, height);
		auto widths = FeatureTracker::TrackCrackWidthProfiles(polygons);
		io::WriteCSV(settings.widths_output.c_str(), widths);
	}
}

// Save output images if output path provided
void saveOutputImages(const Settings &settings, std::vector<uint32_t *> &images,
		      int width, int height) {
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
			sprintf(filename, "%s/image_%d.tif", outputPath.c_str(),
				i);
			io::WriteTiff(filename, images[i], width, height);
		}
	}
}

// Main CLI entry point
void run(int argc, char **argv) {
	// Check for assets folder
	if (!std::filesystem::exists("assets")) {
		fprintf(
		    stderr,
		    "WARNING: assets folder not found, denoising/deformation "
		    "analysis will not work properly\n");
	}

	// Parse command line arguments
	Settings settings = parseArguments(argc, argv);

	// Validate settings
	if (!validateSettings(settings, argv)) {
		return;
	}

	// Load images
	std::vector<uint32_t *> images;
	int width, height;
	if (!loadImages(settings, images, width, height)) {
		return;
	}

	// Apply image processing operations
	applyDenoising(settings, images, width, height);
	performAnalysis(settings, images, width, height);
	calculateWidths(settings, images, width, height);
	saveOutputImages(settings, images, width, height);

	// Check if any operations were performed
	if (!settings.do_crop && !settings.do_denoise && !settings.do_analyze &&
	    !settings.do_widths && settings.output.empty()) {
		printf("Nothing to do. Use --crop and/or --denoise and/or "
		       "--analyze and/or "
		       "--calculate-widths and/or --output.\n");
	}
}

} // namespace cli
