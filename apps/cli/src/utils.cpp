#include <utils.h>

#include <fstream>
#include <stdio.h>
#include <string.h>

#include <opencv2/opencv.hpp>
#include <tiffio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // i love this
#define NOMINMAX	    // i hate this
#include <shobjidl.h>
#include <windows.h>
#endif

namespace utils {
std::string OpenFileDialog(const char *open_path, const char *title, const bool folders_only, const char *filter) {
#ifdef _WIN32
	CoInitialize(nullptr);
	IFileDialog *pFileDialog = nullptr;
	std::wstring folderPath;

	if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileDialog,
				       reinterpret_cast<void **>(&pFileDialog)))) {
		DWORD dwOptions;
		if (SUCCEEDED(pFileDialog->GetOptions(&dwOptions))) {
			pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS);
		}

		if (SUCCEEDED(pFileDialog->Show(nullptr))) {
			IShellItem *pItem;
			if (SUCCEEDED(pFileDialog->GetResult(&pItem))) {
				PWSTR pszFilePath;
				if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
					folderPath = pszFilePath;
					CoTaskMemFree(pszFilePath);
				}
				pItem->Release();
			}
		}
		pFileDialog->Release();
	}

	CoUninitialize();
	if (folderPath.empty())
		return std::string();

	int size_needed = WideCharToMultiByte(CP_UTF8, 0, folderPath.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string str(size_needed - 1, 0);
	WideCharToMultiByte(CP_UTF8, 0, folderPath.c_str(), -1, &str[0], size_needed, nullptr, nullptr);
	return str;
#else

	char buf[256];
	if (folders_only)
		snprintf(buf, 256,
			 "zenity --file-selection --title=\"%s\" --directory "
			 "--filename=%s/",
			 title, open_path);
	else
		snprintf(buf, 256, "zenity --file-selection --title=\"%s\" --filename=%s/", title, open_path);
	char output[1024];
	// open the zenity window
	FILE *f = popen(buf, "r");
	// get filename from zenity
	auto out = fgets(output, 1024, f);
	// if no filename was returned, return an empty string
	// if we don't do this check we get garbage data into the string when
	// the user cancels the dialog
	if (out == nullptr)
		return std::string();
	output[strcspn(output, "\n")] = 0;
	if (output[0] == 0)
		return std::string();
	else
		return std::string(output);
#endif
}

std::string SaveFileDialog(const char *save_path, const char *title, const char *filter) {
#ifdef _WIN32
	CoInitialize(nullptr);
	IFileDialog *pFileDialog = nullptr;
	std::wstring folderPath;
	if (SUCCEEDED(CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL, IID_IFileDialog,
				       reinterpret_cast<void **>(&pFileDialog)))) {
		DWORD dwOptions;
		if (SUCCEEDED(pFileDialog->GetOptions(&dwOptions))) {
			pFileDialog->SetOptions(dwOptions | FOS_OVERWRITEPROMPT);
		}
		if (SUCCEEDED(pFileDialog->Show(nullptr))) {
			IShellItem *pItem;
			if (SUCCEEDED(pFileDialog->GetResult(&pItem))) {
				PWSTR pszFilePath;
				if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
					folderPath = pszFilePath;
					CoTaskMemFree(pszFilePath);
				}
				pItem->Release();
			}
		}
		pFileDialog->Release();
	}
	CoUninitialize();
	if (folderPath.empty())
		return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, folderPath.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string str(size_needed - 1, 0);
	WideCharToMultiByte(CP_UTF8, 0, folderPath.c_str(), -1, &str[0], size_needed, nullptr, nullptr);
	return str;
#else
	char buf[256];
	snprintf(buf, 256, "zenity --file-selection --save --title=\"%s\" --filename=%s", title, save_path);
	char output[1024];
	FILE *f = popen(buf, "r");
	auto out = fgets(output, 1024, f);
	// if we don't do this check we get garbage data into the string when
	// the user cancels the dialog
	if (out == nullptr)
		return std::string();
	output[strcspn(output, "\n")] = 0;
	if (output[0] == 0)
		return std::string();
	else
		return std::string(output);
#endif
}

bool DirectoryContainsTiff(const std::filesystem::path &path) {
	for (auto &it : std::filesystem::directory_iterator(path))
		if (it.path().string().find(".tif") != std::string::npos)
			return true;
	return false;
}

} // namespace utils

namespace io {
unsigned int *LoadTiff(const char *path, int &width, int &height) {
	PROFILE_FUNCTION();

	// set the warning handler to null to avoid printing warnings
	// errors are still printed
	TIFFSetWarningHandler(nullptr);
	TIFF *tif = TIFFOpen(path, "r");
	if (!tif) {
		printf("Could not open file %s\n", path);
		return NULL;
	}
	size_t npixels;
	uint32_t *raster;
	size_t samplesperpixel;

	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
	TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);

	TIFFRGBAImage img;
	char emsg[1024];
	if (!TIFFRGBAImageBegin(&img, tif, 0, emsg)) {
		TIFFError(path, "%s", emsg);
		TIFFClose(tif);
		return NULL;
	}
	npixels = width * height;
	raster = (uint32_t *)_TIFFmalloc(npixels * sizeof(uint32_t));
	if (raster) {
		if (TIFFRGBAImageGet(&img, raster, width, height)) {
			// flip the image
			uint32_t *temp = (uint32_t *)_TIFFmalloc(npixels * sizeof(uint32_t));
			for (int i = 0; i < height; i++) {
				memcpy(temp + i * width, raster + (height - i - 1) * width, width * sizeof(uint32_t));
			}
			TIFFRGBAImageEnd(&img);
			_TIFFfree(raster);
			TIFFClose(tif);
			return temp;
		}
	}
	TIFFRGBAImageEnd(&img);
	_TIFFfree(raster);
	TIFFClose(tif);
	return NULL;
}

bool LoadTiffFolder(const char *folder_path, std::vector<uint32_t *> &images, int &width, int &height) {
	PROFILE_FUNCTION();

	if (!std::filesystem::exists(folder_path)) {
		printf("Path does not exist\n");
		return false;
	}

	// find all .tif files in the folder
	std::vector<std::string> files;
	for (const auto &entry : std::filesystem::directory_iterator(folder_path)) {
		if (entry.path().string().find(".tif") == std::string::npos)
			continue;
		files.push_back(entry.path().string());
	}

	// sort the files by name
	std::sort(files.begin(), files.end());
	for (const auto &file : files) {
		PROFILE_SCOPE(LoadTiffFolderLoop);

		uint32_t *temp = io::LoadTiff(file.c_str(), width, height);
		if (!temp) {
			printf("Could not load file %s\n", file.c_str());
			return false;
		}

		images.push_back(temp);
	}
	return true;
}

bool WriteTiff(const char *path, unsigned int *data, int width, int height) {
	PROFILE_FUNCTION()

	TIFF *tif = TIFFOpen(path, "w");
	if (!tif) {
		printf("Could not open file %s\n", path);
		return false;
	}

	TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
	TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
	TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 4);
	TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
	TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);

	for (int i = 0; i < height; i++) {
		if (TIFFWriteScanline(tif, data + i * width, i, 0) < 0) {
			printf("Error writing tiff\n");
			TIFFClose(tif);
			return false;
		}
	}

	TIFFClose(tif);
	return true;
}

// todo: fix these functions to be good, and not bad. (add better names + headers)
bool WriteCSV(const char *path, std::vector<std::vector<std::vector<float>>> &data) {
	FILE *f = fopen(path, "w");
	for (int i = 0; i < data.size(); i++) {
		if (data[i].empty() || data[i][0].empty()) {
			printf("Data at index %d is empty, skipping\n", i);
			continue;
		}
		if (i > 0)
			fprintf(f, "\n"); // separate frames with a newline
		for (int j = 0; j < data[i][0].size(); j++) {
			fprintf(f, "%f", data[i][0][j]);
			if (j < data[i][0].size() - 1)
				fprintf(f, ",");
		}
	}
	if (!f) {
		printf("Could not open file %s\n", path);
		return false;
	}
	for (int i = 0; i < data.size(); i++) {
		for (int j = 0; j < data[i][0].size(); j++) {
			fprintf(f, "%f", data[i][0][j]);
			if (j < data[i][0].size() - 1)
				fprintf(f, ",");
		}
		fprintf(f, "\n");
	}
	fclose(f);
	return true;
}

bool WriteCSV(const char *path, std::vector<std::vector<cv::Point2f>> &trackedPts,
	      std::vector<std::vector<float>> &widths) {
	std::ofstream f(path);
	if (!f.is_open()) {
		std::cerr << "Could not open file " << path << std::endl;
		return false;
	}
	// header
	f << "frame";
	auto nPairs = widths.empty() ? 0 : widths[0].size();
	for (size_t p = 0; p < nPairs; ++p)
		f << ",width" << p;
	auto nPts = trackedPts.empty() ? 0 : trackedPts[0].size();
	for (size_t j = 0; j < nPts; ++j)
		f << ",pt" << j << "_x,pt" << j << "_y";
	f << "\n";

	auto nFrames = std::min(widths.size(), trackedPts.size());
	for (size_t i = 0; i < nFrames; ++i) {
		f << i;
		for (auto w : widths[i])
			f << "," << w;
		for (auto &pt : trackedPts[i])
			f << "," << pt.x << "," << pt.y;
		f << "\n";
	}
	return true;
}

bool SaveAnalysisCsv(const char *path, const std::vector<std::vector<float>> &histograms,
		     const std::vector<float> &avg_histogram, const std::vector<float> &snrs, float avg_snr) {
	std::ofstream f(path);
	if (!f.is_open()) {
		std::cerr << "Could not open file " << path << std::endl;
		return false;
	}
	int bins = avg_histogram.size();

	// header
	f << "frame,snr";
	for (int b = 0; b < bins; ++b)
		f << ",bin" << b;
	f << "\n";

	// per-frame rows
	size_t n = std::min(histograms.size(), snrs.size());
	for (size_t i = 0; i < n; ++i) {
		f << i << "," << snrs[i];
		for (int b = 0; b < bins; ++b)
			f << "," << histograms[i][b];
		f << "\n";
	}

	// average row
	f << "avg," << avg_snr;
	for (int b = 0; b < bins; ++b)
		f << "," << avg_histogram[b];
	f << "\n";
	return true;
}
} // namespace io

Profiler::Profiler(const char *name) {
	start_time = std::chrono::high_resolution_clock::now();
	this->name = name;
}

Profiler::~Profiler() {
	if (!stopped)
		Stop();
}

void Profiler::Stop() {
	auto end_time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
	printf("Profiler: %s took %zu ms\n", name, duration);
	stopped = true;
}
