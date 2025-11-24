#include <algorithm>
#include <charconv>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// project headers
#include <deformation_core/CrackDetector.hpp>
#include <deformation_core/DenoiseInterface.hpp>
#include <deformation_core/FeatureTracker.hpp>
#include <deformation_core/ImageAnalysis.hpp>
#include <deformation_core/Stabilizer.hpp>
#include <utils.h>

#include "cli.hpp"

namespace fs = std::filesystem;
using std::string;
using std::string_view;
using std::vector;

namespace {
// --------- small helpers ---------
[[noreturn]] void die(string_view msg) {
	std::cerr << "error: " << msg
		  << "";
	    std::exit(1);
}

int to_int(string_view s, string_view what) {
	int v{};
	auto *first = s.data();
	auto *last = s.data() + s.size();
	if (std::from_chars(first, last, v).ec != std::errc{})
		die(std::format("invalid integer for {}: {}", what, s));
	return v;
}

string_view take_value(const vector<string_view> &argv, size_t &i, string_view flag) {
	if (i + 1 >= argv.size())
		die(std::format("{} requires a value", flag));
	return argv[++i];
}

// --------- usage text ---------
constexpr string_view BANNER = "enhancing-deformation analysis (eda) cli";

constexpr string_view USAGE = R"USAGE(

usage (subcommands):
  eda --folder <path> [--output <dir>] [--verbose] [--quiet] [--no-color]
     [crop --pixels <n>]
     [denoise --filter <name> --tile <n> --overlap <n> --center <n> [--include-outside]]
     [analyze --stats <stats.csv>]
     [motion --out <motion.csv> [--features <n>]]
     [widths --out <widths.csv>]

examples:
  eda --folder data crop --pixels 16 denoise --filter blur --tile 256 --overlap 0 --center 64 --include-outside
  eda --folder data analyze --stats stats.csv widths --out widths.csv --output out
  eda --folder data/sequence motion --out motion.csv --features 100 --output results
)USAGE";

// --------- domain-facing pieces ---------
struct Context {
	string folder;
	vector<uint32_t *> images;
	int w = 0, h = 0;
	bool verbose = false;
	bool quiet = false;
};

struct CropArgs {
	int pixels = 0;
};
struct DenoiseArgs {
	string filter;
	int tile = 256;
	int overlap = 0;
	int center = 64;
	bool include_outside = false;
};
struct AnalyzeArgs {
	string stats_csv;
};
struct WidthsArgs {
	string widths_csv;
};

struct MotionArgs {
    string output_csv;
    int num_features = 50;  // automatically detected the number of feature points
};

void load_images(Context &ctx) {
	if (!fs::exists(ctx.folder))
		die(std::format("folder not found: {}", ctx.folder));
	if (!io::LoadTiffFolder(ctx.folder.c_str(), ctx.images, ctx.w, ctx.h))
		die("failed to load images");
	if (ctx.verbose) std::cerr << std::format("loaded {} images ({}x{})", ctx.images.size(), ctx.w, ctx.h);
}

void apply_crop(Context &ctx, const CropArgs &a) {
	if (a.pixels < 0 || a.pixels >= ctx.h)
		die(std::format("crop pixels {} out of range [0, {})", a.pixels, ctx.h));
	ctx.h -= a.pixels;
	if (ctx.verbose) std::cerr << std::format("cropped {} px from bottom -> new size ({}x{})", a.pixels, ctx.w, ctx.h);
}

void apply_denoise(Context &ctx, const DenoiseArgs &a) {
	static const vector<string> valid = {"blur",	  "sfr_hrsem",	"sfr_hrstem", "sfr_hrtem",
					     "sfr_lrsem", "sfr_lrstem", "sfr_lrtem"};
	if (std::find(valid.begin(), valid.end(), a.filter) == valid.end())
		die(std::format("invalid denoise filter: {}", a.filter));
	TileConfig cfg(TileType::Cropped, a.tile, a.overlap, a.center, a.include_outside);
	if (a.filter == "blur") {
		DenoiseInterface::Blur(ctx.images, ctx.w, ctx.h, 3, 1.0f);
	} else {
		DenoiseInterface::Denoise(ctx.images, ctx.w, ctx.h, a.filter, cfg);
	}
	if (ctx.verbose)
		std::cerr << std::format("denoise done (filter={}, tile={}, ov={}, center={}, include_outside={})",
					 a.filter,
					 a.tile, a.overlap, a.center, a.include_outside);
}

void run_analysis(Context &ctx, const AnalyzeArgs &a) {
	vector<vector<float>> hists;
	vector<float> avg;
	vector<float> snrs;
	float avg_snr = 0.0f;
	ImageAnalysis::AnalyzeImages(ctx.images, ctx.w, ctx.h, hists, avg, snrs, avg_snr);
	io::SaveAnalysisCsv(a.stats_csv.c_str(), hists, avg, snrs, avg_snr);
	if (ctx.verbose) std::cerr << std::format("analysis -> {} (avg snr={})", a.stats_csv, avg_snr);
}

void run_widths(Context &ctx, const WidthsArgs &a) {
	auto polys = CrackDetector::DetectCracks(ctx.images, ctx.w, ctx.h);
	auto widths = FeatureTracker::TrackCrackWidthProfiles(polys);
	io::WriteCSV(a.widths_csv.c_str(), widths);
	if (ctx.verbose) std::cerr << std::format("widths -> {}", a.widths_csv);
}

void run_motion(Context &ctx, const MotionArgs &a) {
    if (ctx.images.size() < 2) {
        die("motion tracking requires at least 2 images");
    }
    
    // detect feature points in the first frame
    cv::Mat firstFrame(ctx.h, ctx.w, CV_8UC4, ctx.images[0]);
    cv::Mat gray;
    cv::cvtColor(firstFrame, gray, cv::COLOR_BGRA2GRAY);
    
    std::vector<cv::Point2f> points;
    cv::goodFeaturesToTrack(gray, points, a.num_features, 0.01, 10);
    
    if (points.empty()) {
        die("no features detected in first frame");
    }
    
    if (ctx.verbose) {
        std::cerr << std::format("detected {} feature points", points.size());
    }
    
    // Call the current TrackFeatures functions
    std::vector<std::vector<cv::Point2f>> tracked_points;
    auto widths = FeatureTracker::TrackFeatures(
        ctx.images, points, tracked_points, ctx.w, ctx.h
    );
    
    // save to csv
    io::WriteMotionCSV(a.output_csv.c_str(), tracked_points);
    
    if (ctx.verbose) {
        std::cerr << std::format("motion tracking -> {}", a.output_csv);
    }
}

void save_outputs(Context &ctx, const string &outdir) {
	fs::create_directories(outdir);
	for (size_t i = 0; i < ctx.images.size(); ++i) {
		auto fn = std::format("{}/image_{}.tif", outdir, i);
		io::WriteTiff(fn.c_str(), ctx.images[i], ctx.w, ctx.h);
	}
	if (ctx.verbose) std::cerr << std::format("wrote {} images -> {}", ctx.images.size(), outdir);
}

// --------- argument parsing ---------
struct PipelineStep {
    enum Kind { Crop, Denoise, Analyze, Widths, Motion } kind;
    CropArgs crop;
    DenoiseArgs denoise;
    AnalyzeArgs analyze;
    WidthsArgs widths;
    MotionArgs motion;
};

struct Cmdline {
	string folder;
	string output;
	bool verbose = false;
	bool quiet = false;
	vector<PipelineStep> steps; // keep order user provided
};

void print_help_and_exit() {
	std::cout << BANNER
		  << "" << USAGE << "";
	    std::exit(0);
}

Cmdline parse_args(const vector<string_view> &args) {
	Cmdline c{};
	for (size_t i = 0; i < args.size(); ++i) {
		auto a = args[i];
		if (a == "-h" || a == "--help")
			print_help_and_exit();
		if (a == "--version") {
			std::cout << BANNER
				  << ""; std::exit(0); }
			    if (a == "--verbose") {
				c.verbose = true;
				continue;
			}
			if (a == "--quiet") {
				c.quiet = true;
				continue;
			}
			if (a == "--no-color") { /* reserved; no colors emitted rn */
				continue;
			}
			if (a == "--folder") {
				c.folder = string(take_value(args, i, a));
				continue;
			}
			if (a == "--output") {
				c.output = string(take_value(args, i, a));
				continue;
			}

			if (a == "crop") {
				PipelineStep s{PipelineStep::Crop};
				bool seen = false;
				while (i + 1 < args.size() && args[i + 1].starts_with("--")) {
					auto k = args[++i];
					if (k == "--pixels") {
						s.crop.pixels = to_int(take_value(args, i, k), k);
						seen = true;
					} else {
						--i;
						break;
					}
				}
				if (!seen)
					die("crop requires --pixels <n>");
				c.steps.push_back(s);
				continue;
			}
			if (a == "denoise") {
				PipelineStep s{PipelineStep::Denoise};
				while (i + 1 < args.size() && args[i + 1].starts_with("--")) {
					auto k = args[++i];
					if (k == "--filter")
						s.denoise.filter = string(take_value(args, i, k));
					else if (k == "--tile")
						s.denoise.tile = to_int(take_value(args, i, k), k);
					else if (k == "--overlap")
						s.denoise.overlap = to_int(take_value(args, i, k), k);
					else if (k == "--center")
						s.denoise.center = to_int(take_value(args, i, k), k);
					else if (k == "--include-outside")
						s.denoise.include_outside = true;
					else {
						--i;
						break;
					}
				}
				if (s.denoise.filter.empty())
					die("denoise requires --filter <name>");
				c.steps.push_back(s);
				continue;
			}
			if (a == "analyze") {
				PipelineStep s{PipelineStep::Analyze};
				if (!(i + 2 < args.size() && args[i + 1] == string_view{"--stats"}))
					die("analyze requires --stats <file>");
				i += 1;
				s.analyze.stats_csv = string(take_value(args, i, "--stats"));
				c.steps.push_back(s);
				continue;
			}
			if (a == "widths") {
				PipelineStep s{PipelineStep::Widths};
				if (!(i + 2 < args.size() && args[i + 1] == string_view{"--out"}))
					die("widths requires --out <file>");
				i += 1;
				s.widths.widths_csv = string(take_value(args, i, "--out"));
				c.steps.push_back(s);
				continue;
			}

			if (a == "motion") {
				PipelineStep s{PipelineStep::Motion};
				
				while (i + 1 < args.size() && args[i + 1].starts_with("--")) {
					auto k = args[++i];
					if (k == "--out") {
						s.motion.output_csv = string(take_value(args, i, k));
					} else if (k == "--features") {
						s.motion.num_features = to_int(take_value(args, i, k), k);
					} else {
						--i;
						break;
					}
				}
				if (s.motion.output_csv.empty()) {
					die("motion requires --out <file>");
				}
				c.steps.push_back(s);
				continue;
			}

	die(std::format("unknown argument: {} {}", a, USAGE));
		}

		if (c.folder.empty())
			die("--folder is required");
		return c;
	}

	int execute(const Cmdline &cmd) {
		Context ctx;
		ctx.folder = cmd.folder;
		ctx.verbose = cmd.verbose;
		ctx.quiet = cmd.quiet;
		load_images(ctx);

		if (cmd.steps.empty() && cmd.output.empty()) {
			if (!cmd.quiet)
				std::cout << "nothing to do. see --help for usage.";
				    return 0;
		}

		for (const auto &s : cmd.steps) {
			switch (s.kind) {
			case PipelineStep::Crop:
				apply_crop(ctx, s.crop);
				break;
			case PipelineStep::Denoise:
				apply_denoise(ctx, s.denoise);
				break;
			case PipelineStep::Analyze:
				run_analysis(ctx, s.analyze);
				break;
			case PipelineStep::Widths:
				run_widths(ctx, s.widths);
				break;
			case PipelineStep::Motion:
				run_motion(ctx, s.motion);
				break;
			}
		}

		if (!cmd.output.empty())
			save_outputs(ctx, cmd.output);
		return 0;
	}

} // namespace (anon)

// public api
namespace cli {

int run_argv(const vector<string> &args) {
	vector<string_view> views;
	views.reserve(args.size());
	for (auto &s : args)
		views.push_back(s);
	auto cmd = parse_args(views);
	return execute(cmd);
}

void run(int argc, char **argv) {
	vector<string_view> args;
	for (int i = 1; i < argc; ++i)
		args.emplace_back(argv[i]);
	if (args.empty()) {
		print_help_and_exit();
	}
	auto cmd = parse_args(args);
	(void)execute(cmd);
}

} // namespace cli
