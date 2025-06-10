#include <ui/ImageSet.h>

#include <core/CrackDetector.hpp>
#include <core/DeformationAnalysisInterface.hpp>
#include <core/DenoiseInterface.hpp>
#include <core/FeatureTracker.hpp>
#include <core/ImageAnalysis.hpp>

#include <utils.h>

#include <imgui.h>

#include <opencv2/opencv.hpp>

#include <algorithm>
#include <format>
#include <string>

#define CALC_SLIDER_SIZE(text) (ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(#text).x) - 5

int ImageSet::m_id_counter = 0;

ImageSet::ImageSet(const std::string_view &folder_path) : m_folder_path(folder_path) {
	m_window_id = m_id_counter++;
	m_window_name = folder_path.find_last_of('/') == std::string::npos
			    ? folder_path
			    : folder_path.substr(folder_path.find_last_of('/') + 1);
	m_window_name = std::format("{} {}", m_window_name, m_window_id);

	LoadImages();

	m_point_texture = Texture();

	m_preprocessing_tab = PreprocessingTab(m_textures, m_processed_textures);
}

ImageSet::~ImageSet() { free(m_point_image); }

// display the image set window and the tabs
void ImageSet::Display() {
	ImGui::Begin(m_window_name.c_str(), &m_open);

	// Check if processing is happening in the preprocessing tab
	bool isPreprocessProcessing = m_preprocessing_tab.IsProcessing();
	bool isDeformationProcessing = DeformationAnalysisInterface::IsProcessing();

	// Check if an async processing task has completed
	if (!isDeformationProcessing && m_processing_future && m_processing_future->valid()) {
		// Poll the future with zero timeout to check if it's done without blocking
		auto status = m_processing_future->wait_for(std::chrono::seconds(0));
		if (status == std::future_status::ready) {
			auto result = m_processing_future->get();
			m_model_ok = result;

			// Create textures for the output tiles
			for (auto &tile : m_output_tiles) {
				// Make sure we have valid image data
				if (tile.data.data && tile.data.cols > 0 && tile.data.rows > 0) {
					m_output_tile_textures.push_back(std::make_shared<Texture>());
					m_output_tile_textures.back()->Load((uint32_t *)tile.data.data, tile.data.cols,
									    tile.data.rows);
				}
			}

			// Create full image texture if tiles are available
			if (!m_output_tiles.empty() && !m_processed_textures.empty()) {
				m_full_image_texture = std::make_shared<Texture>();
				m_full_image_texture->Load(m_processing_frames[0], m_processed_textures[0]->GetWidth(),
							   m_processed_textures[0]->GetHeight());
			}

			utils::LoadDataIntoTexturesAndFree(m_processed_textures, m_processing_frames,
							   m_processed_textures[0]->GetWidth(),
							   m_processed_textures[0]->GetHeight());

			m_processing_frames.clear();
		}
	}

	// Start tab bar
	ImGui::BeginTabBar("Image Set Tabs");

	bool changed = false;

	// If not processing, show the image comparison tab
	if (!isPreprocessProcessing && !isDeformationProcessing) {
		DisplayImageComparisonTab();
	}

	if (isPreprocessProcessing || isDeformationProcessing) {
		ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.8f, 0.5f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.9f, 0.6f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(1.0f, 0.7f, 0.0f, 1.0f));
	}

	// if we aren't doing deformation analysis, show the preprocessing tab
	if (!isDeformationProcessing)
		m_preprocessing_tab.DisplayPreprocessingTab(changed);

	if (isPreprocessProcessing || isDeformationProcessing) {
		ImGui::PopStyleColor(3);
	}

	// if we aren't doing preprocessing, show the deformation analysis tab
	if (!isPreprocessProcessing)
		DisplayDeformationAnalysisTab();

	// Display the other tabs only if not processing
	if (!isPreprocessProcessing && !isDeformationProcessing) {
		DisplayFeatureTrackingTab();
		DisplayImageAnalysisTab();
	}

	// only necessary when we modify the vector by changing its size (this
	// only happens in the preprocessing tab)
	if (changed) {
		m_preprocessing_tab.GetProcessedTextures(m_processed_textures);
	}

	ImGui::EndTabBar();
	ImGui::End();
}

void ImageSet::LoadImages() {
	PROFILE_FUNCTION();

	std::vector<uint32_t *> images;
	int width, height;
	io::LoadTiffFolder(m_folder_path.c_str(), images, width, height);

	for (auto &image : images) {
		std::shared_ptr<Texture> t = std::make_shared<Texture>();
		t->Load(image, width, height);
		m_textures.push_back(t);
		std::shared_ptr<Texture> t2 = std::make_shared<Texture>();
		t2->Load(image, width, height);
		m_processed_textures.push_back(t2);
		free(image);
	}
}

// TODO: change to incorporate the original images and images from
// preprocessing, feature tracking, and deformation analysis (all separate)
// or maybe not?
void ImageSet::DisplayImageComparisonTab() {
	static bool isPlaying = false;
	if (ImGui::BeginTabItem("Image Comparison")) {
		// Create a window that fills the tab area and has a menu bar
		ImGui::BeginChild("ImageComparisonTab", ImVec2(0, 0), false, ImGuiWindowFlags_MenuBar);

		// Add a menubar at the top of the tab content
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				// Export options for the current frame
				if (ImGui::BeginMenu("Export Current Frame")) {
					if (ImGui::MenuItem("Original as TIFF")) {
						if (!m_textures.empty() && m_current_frame < m_textures.size()) {
							std::string path = utils::SaveFileDialog(".",
												 "Save "
												 "Original "
												 "Frame as "
												 "TIFF",
												 "tif");
							if (!path.empty()) {
								uint32_t *data = new uint32_t
								    [m_textures[m_current_frame]->GetWidth() *
								     m_textures[m_current_frame]->GetHeight()];
								m_textures[m_current_frame]->GetData(data);
								io::WriteTiff(path.c_str(), data,
									      m_textures[m_current_frame]->GetWidth(),
									      m_textures[m_current_frame]->GetHeight());
								delete[] data;
							}
						}
					}

					if (ImGui::MenuItem("Processed as TIFF")) {
						if (!m_processed_textures.empty() &&
						    m_current_frame < m_processed_textures.size()) {
							std::string path = utils::SaveFileDialog(".",
												 "Save "
												 "Processed "
												 "Frame as "
												 "TIFF",
												 "tif");
							if (!path.empty()) {
								uint32_t *data =
								    new uint32_t[m_processed_textures[m_current_frame]
										     ->GetWidth() *
										 m_processed_textures[m_current_frame]
										     ->GetHeight()];
								m_processed_textures[m_current_frame]->GetData(data);
								io::WriteTiff(
								    path.c_str(), data,
								    m_processed_textures[m_current_frame]->GetWidth(),
								    m_processed_textures[m_current_frame]->GetHeight());
								delete[] data;
							}
						}
					}
					ImGui::EndMenu();
				}

				// Export options for all frames
				if (ImGui::BeginMenu("Export All Frames")) {
					if (ImGui::MenuItem("Original Sequence as TIFF")) {
						std::string folder = utils::OpenFileDialog(".",
											   "Choose a Folder to "
											   "Save Original Images",
											   true);
						if (!folder.empty() && !m_textures.empty()) {
							uint32_t *data = new uint32_t[m_textures[0]->GetWidth() *
										      m_textures[0]->GetHeight()];
							for (int i = 0; i < m_textures.size(); i++) {
								char path[256];
								sprintf(path,
									"%s/"
									"original_"
									"frame_%d."
									"tif",
									folder.c_str(), i);
								m_textures[i]->GetData(data);
								io::WriteTiff(path, data, m_textures[i]->GetWidth(),
									      m_textures[i]->GetHeight());
							}
							delete[] data;
						}
					}

					if (ImGui::MenuItem("Processed Sequence as TIFF")) {
						std::string folder = utils::OpenFileDialog(".",
											   "Choose a Folder to "
											   "Save Processed Images",
											   true);
						if (!folder.empty() && !m_processed_textures.empty()) {
							uint32_t *data =
							    new uint32_t[m_processed_textures[0]->GetWidth() *
									 m_processed_textures[0]->GetHeight()];
							for (int i = 0; i < m_processed_textures.size(); i++) {
								char path[256];
								sprintf(path,
									"%s/"
									"processed_"
									"frame_%d."
									"tif",
									folder.c_str(), i);
								m_processed_textures[i]->GetData(data);
								io::WriteTiff(path, data,
									      m_processed_textures[i]->GetWidth(),
									      m_processed_textures[i]->GetHeight());
							}
							delete[] data;
						}
					}

					if (ImGui::MenuItem("Processed Sequence as GIF")) {
						std::string path = utils::SaveFileDialog(".",
											 "Save Processed "
											 "Sequence as GIF",
											 "gif");
						if (!path.empty() && !m_processed_textures.empty()) {
							io::WriteGIFOfImageSet(path.c_str(), m_processed_textures, 40,
									       0);
						}
					}
					ImGui::EndMenu();
				}
				ImGui::Separator();
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		ImGui::BeginChild("Controls", ImVec2(250, 0), true);

		// Playback controls section
		ImGui::SeparatorText("Playback Controls");

		// Frame navigation controls
		int max_frame = (int)std::max(m_textures.size(), m_processed_textures.size()) - 1;

		// Keep within bounds if frames were removed
		if (m_current_frame > max_frame)
			m_current_frame = max_frame;

		// Frame counter with slider
		ImGui::Text("Frame: %d/%d", m_current_frame + 1, max_frame + 1);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::SliderInt("##FrameSlider", (int *)&m_current_frame, 0, max_frame, "")) {
			// Frame changed - handle any necessary state updates
		}

		// Create a row of navigation controls
		float button_width = ImGui::GetContentRegionAvail().x / 3 - 5;

		// Previous frame button
		ImGui::BeginDisabled(m_current_frame <= 0);
		if (ImGui::Button("Previous", ImVec2(button_width, 0))) {
			m_current_frame--;
		}
		ImGui::EndDisabled();

		ImGui::SameLine();

		// Play/Pause button
		if (ImGui::Button(isPlaying ? "Pause" : "Play", ImVec2(button_width, 0))) {
			isPlaying = !isPlaying;
		}

		ImGui::SameLine();

		// Next frame button
		ImGui::BeginDisabled(m_current_frame >= max_frame);
		if (ImGui::Button("Next", ImVec2(button_width, 0))) {
			m_current_frame++;
		}
		ImGui::EndDisabled();

		// Speed control
		static float playback_speed = 10.0f; // frames per second
		ImGui::SetNextItemWidth(CALC_SLIDER_SIZE(Speed));
		ImGui::SliderFloat("Speed", &playback_speed, 1.0f, 30.0f, "%.1f fps");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Frames per second");
		}

		// Reset playback button
		if (ImGui::Button("Reset Playback", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
			m_current_frame = 0;
			isPlaying = false;
		}

		// Image options section
		ImGui::SeparatorText("Image Options");

		// Display scaling slider
		static float display_scale = 1.5f;
		ImGui::SetNextItemWidth(CALC_SLIDER_SIZE(Scale));
		ImGui::SliderFloat("Scale", &display_scale, 1.0f, 3.0f, "%.1fx");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Adjust image display size");
		}

		// Reset processed images button
		if (ImGui::Button("Reset Processed Images", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
			PROFILE_SCOPE(ResetProcessedImages);

			while (m_textures.size() > m_processed_textures.size())
				m_processed_textures.push_back(std::make_shared<Texture>());
			ImVec2 size = ImVec2(m_textures[0]->GetWidth(), m_textures[0]->GetHeight());
			uint32_t *data = (uint32_t *)malloc(size.x * size.y * sizeof(uint32_t));
			for (int i = 0; i < m_textures.size(); i++) {
				m_textures[i]->GetData(data);
				m_processed_textures[i]->Load(data, (int)size.x, (int)size.y);
			}
			m_preprocessing_tab.SetProcessedTextures(m_processed_textures);
			free(data);
		}

		// Help section
		ImGui::SeparatorText("Help");
		ImGui::TextWrapped("Tip: Use the File menu above for export options.");

		ImGui::EndChild();

		ImGui::SameLine();

		// Side-by-side image display with improved styling
		ImGui::BeginChild("Images", ImVec2(0, 0), true);

		// Image navigation bar at the top
		if (!m_textures.empty() && !m_processed_textures.empty()) {
			// Frame navigation strip at the top
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

			// Left/right buttons
			ImGui::BeginDisabled(m_current_frame <= 0);
			if (ImGui::ArrowButton("##left_top", ImGuiDir_Left))
				m_current_frame--;
			ImGui::EndDisabled();

			ImGui::SameLine();

			// Frame slider (more compact version for the top)
			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 70);
			if (ImGui::SliderInt("##FrameSliderTop", (int *)&m_current_frame, 0, max_frame, "Frame %d")) {
				// Frame changed
			}
			ImGui::PopItemWidth();

			ImGui::SameLine();

			ImGui::BeginDisabled(m_current_frame >= max_frame);
			if (ImGui::ArrowButton("##right_top", ImGuiDir_Right))
				m_current_frame++;
			ImGui::EndDisabled();

			ImGui::SameLine();

			// Play/Pause button
			if (ImGui::Button(isPlaying ? "⏸" : "▶")) {
				isPlaying = !isPlaying;
			}

			ImGui::PopStyleVar();

			ImGui::Separator();
		}

		// Image display area
		ImGui::Columns(2);

		// Left sequence (original)
		ImGui::SeparatorText("Original Sequence");
		if (!m_textures.empty() && m_current_frame < m_textures.size()) {
			// Calculate the available size
			ImVec2 avail = ImGui::GetContentRegionAvail();
			ImVec2 img_size = ImVec2(m_textures[0]->GetWidth(), m_textures[0]->GetHeight());
			float aspect = img_size.x / img_size.y;

			// Scale the image to fit the available width
			ImVec2 display_size = ImVec2(std::min(avail.x, img_size.x / display_scale),
						     std::min(avail.x / aspect, img_size.y / display_scale));

			// Calculate centered position
			ImVec2 cursor_pos = ImGui::GetCursorPos();
			ImVec2 centered_pos = ImVec2(cursor_pos.x + (avail.x - display_size.x) * 0.5f, cursor_pos.y);
			ImGui::SetCursorPos(centered_pos);

			// Display image
			ImGui::Image(m_textures[m_current_frame]->GetID(), display_size);

			// Image information
			ImGui::SetCursorPosX(cursor_pos.x);
			ImGui::TextUnformatted(
			    ("Size: " + std::to_string((int)img_size.x) + "x" + std::to_string((int)img_size.y))
				.c_str());
		}

		ImGui::NextColumn();

		// Right sequence (processed)
		ImGui::SeparatorText("Processed Sequence");
		if (!m_processed_textures.empty() && m_current_frame < m_processed_textures.size()) {
			// Calculate the available size
			ImVec2 avail = ImGui::GetContentRegionAvail();
			ImVec2 img_size =
			    ImVec2(m_processed_textures[0]->GetWidth(), m_processed_textures[0]->GetHeight());
			float aspect = img_size.x / img_size.y;

			// Scale the image to fit the available width
			ImVec2 display_size = ImVec2(std::min(avail.x, img_size.x / display_scale),
						     std::min(avail.x / aspect, img_size.y / display_scale));

			// Calculate centered position
			ImVec2 cursor_pos = ImGui::GetCursorPos();
			ImVec2 centered_pos = ImVec2(cursor_pos.x + (avail.x - display_size.x) * 0.5f, cursor_pos.y);
			ImGui::SetCursorPos(centered_pos);

			// Display image
			ImGui::Image(m_processed_textures[m_current_frame]->GetID(), display_size);

			// Image information
			ImGui::SetCursorPosX(cursor_pos.x);
			ImGui::TextUnformatted(
			    ("Size: " + std::to_string((int)img_size.x) + "x" + std::to_string((int)img_size.y))
				.c_str());
		}

		ImGui::Columns(1);
		ImGui::EndChild();

		// Playback logic with speed control
		if (isPlaying) {
			static float last_time = ImGui::GetTime();
			float current_time = ImGui::GetTime();
			float frame_time = 1.0f / playback_speed;

			if (current_time - last_time > frame_time) {
				m_current_frame++;
				if (m_current_frame >= std::max(m_textures.size(), m_processed_textures.size())) {
					m_current_frame = 0; // Loop back to start
				}
				last_time = current_time;
			}
		}

		// End the child window with the menu bar
		ImGui::EndChild();

		ImGui::EndTabItem();
	}
}

void ImageSet::DisplayImageAnalysisTab() {
	static uint32_t *s_comparison_image = nullptr;
	static uint32_t s_ref_image_width = 0, s_ref_image_height = 0;
	if (ImGui::BeginTabItem("Image Analysis")) {
		if (m_processed_textures.size() == 0) {
			ImGui::Text("No images loaded");
			ImGui::EndTabItem();
			return;
		}

		// Ensure current frame is within bounds
		m_analysis_current_frame =
		    std::clamp(m_analysis_current_frame, 0, (int)m_processed_textures.size() - 1);

		if (s_comparison_image == nullptr ||
		    s_ref_image_width * s_ref_image_height !=
			m_processed_textures[0]->GetWidth() * m_processed_textures[0]->GetHeight()) {
			free(s_comparison_image);
			s_comparison_image = (uint32_t *)malloc(m_processed_textures[0]->GetWidth() *
								m_processed_textures[0]->GetHeight() * 4);
			s_ref_image_width = m_processed_textures[0]->GetWidth();
			s_ref_image_height = m_processed_textures[0]->GetHeight();
		}
		if (m_ref_image == nullptr ||
		    m_ref_image_width * m_ref_image_height !=
			m_processed_textures[0]->GetWidth() * m_processed_textures[0]->GetHeight()) {
			free(m_ref_image);
			m_ref_image =
			    new uint32_t[m_processed_textures[0]->GetWidth() * m_processed_textures[0]->GetHeight()];
			m_ref_image_width = m_processed_textures[0]->GetWidth();
			m_ref_image_height = m_processed_textures[0]->GetHeight();
		}
		m_processed_textures[0]->GetData(s_comparison_image);
		if (memcmp(m_ref_image, s_comparison_image, m_ref_image_width * m_ref_image_height * 4) != 0) {
			m_processed_textures[0]->GetData(m_ref_image);
			histograms.clear();
			avg_histogram.clear();
			snrs.clear();
			avg_snr = 0.0f;
			std::vector<uint32_t *> frames;
			utils::GetDataFromTextures(frames, m_processed_textures[0]->GetWidth(),
						   m_processed_textures[0]->GetHeight(), m_processed_textures);
			ImageAnalysis::AnalyzeImages(frames, m_ref_image_width, m_ref_image_height, histograms,
						     avg_histogram, snrs, avg_snr);
			for (auto &frame : frames) {
				free(frame);
			}
		}

		// Create layout with image on left, controls and histogram on right
		ImGui::BeginChild("AnalysisControls", ImVec2(250, 0));

		ImGui::SeparatorText("Frame Navigation");
		if (m_processed_textures.size() > 1) {
			// Frame slider with text showing current/total frames
			ImGui::Text("Frame: %d/%d", m_analysis_current_frame + 1, (int)m_processed_textures.size());
			ImGui::SetNextItemWidth(220);
			if (ImGui::SliderInt("##AnalysisFrameSlider", &m_analysis_current_frame, 0,
					     m_processed_textures.size() - 1, "")) {
				// Keep within bounds
				m_analysis_current_frame = std::max(
				    0, std::min(m_analysis_current_frame, (int)m_processed_textures.size() - 1));
			}

			// Navigation buttons
			ImGui::BeginDisabled(m_analysis_current_frame <= 0);
			if (ImGui::ArrowButton("##analysis_left", ImGuiDir_Left))
				m_analysis_current_frame--;
			ImGui::EndDisabled();

			ImGui::SameLine();
			ImGui::BeginDisabled(m_analysis_current_frame >= m_processed_textures.size() - 1);
			if (ImGui::ArrowButton("##analysis_right", ImGuiDir_Right))
				m_analysis_current_frame++;
			ImGui::EndDisabled();
		}

		ImGui::SeparatorText("Frame Analysis");
		if (!histograms.empty() && m_analysis_current_frame < histograms.size()) {
			ImGui::Text("SNR: %.2f", snrs[m_analysis_current_frame]);
			auto size = ImVec2(220, 200);
			char label[256];
			sprintf(label, "Frame %d Histogram", m_analysis_current_frame);
			ImGui::PlotHistogram(label, &histograms[m_analysis_current_frame][0], 256, 0, NULL, 0.0f, 1.0f,
					     size);
		}

		ImGui::SeparatorText("Region Analysis");
		if (ImGui::Button(m_region_selection_active ? "Cancel Selection" : "Select Region")) {
			m_region_selection_active = !m_region_selection_active;
			if (!m_region_selection_active) {
				m_region_selected = false;
			}
		}

		if (m_region_selected) {
			ImVec2 region_size =
			    ImVec2(abs(m_region_end.x - m_region_start.x), abs(m_region_end.y - m_region_start.y));
			ImGui::Text("Region: %.0fx%.0f pixels", region_size.x, region_size.y);
			ImGui::Text("Region SNR: %.2f", m_region_snr);

			if (!m_region_histogram.empty()) {
				auto size = ImVec2(220, 150);
				ImGui::PlotHistogram("Region Histogram", &m_region_histogram[0], 256, 0, NULL, 0.0f,
						     1.0f, size);
			}

			if (ImGui::Button("Clear Region")) {
				m_region_selected = false;
				m_region_selection_active = false;
			}
		} else if (m_region_selection_active) {
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Click and drag on image");
		}

		ImGui::SeparatorText("Average Analysis");
		ImGui::Text("Average SNR: %.2f", avg_snr);
		if (!avg_histogram.empty()) {
			auto size = ImVec2(220, 200);
			ImGui::PlotHistogram("Average Histogram", &avg_histogram[0], 256, 0, NULL, 0.0f, 1.0f, size);
		}

		ImGui::SeparatorText("Export");
		if (ImGui::Button("Save Analysis CSV")) {
			auto path = utils::SaveFileDialog(".", "Save Analysis CSV", "csv");
			write_success = io::SaveAnalysisCsv(path.c_str(), histograms, avg_histogram, snrs, avg_snr);
		}
		if (!write_success) {
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error saving analysis!");
		}

		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("ImageView", ImVec2(0, 0));

		// Display the current frame
		ImGui::Text("Current Frame: %d", m_analysis_current_frame);
		ImGui::Separator();

		// Get image dimensions and position
		auto image_size = ImVec2(m_processed_textures[m_analysis_current_frame]->GetWidth(),
					 m_processed_textures[m_analysis_current_frame]->GetHeight());
		ImVec2 image_pos = ImGui::GetCursorScreenPos();

		// Display the image
		ImGui::Image((ImTextureID)m_processed_textures[m_analysis_current_frame]->GetID(), image_size);

		// Handle region selection on the image
		if (m_region_selection_active && ImGui::IsItemHovered()) {
			ImGuiIO &io = ImGui::GetIO();
			ImVec2 mouse_pos = ImVec2(io.MousePos.x - image_pos.x, io.MousePos.y - image_pos.y);

			// Clamp mouse position to image bounds
			mouse_pos.x = std::max(0.0f, std::min(mouse_pos.x, image_size.x));
			mouse_pos.y = std::max(0.0f, std::min(mouse_pos.y, image_size.y));

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				m_region_start = mouse_pos;
				m_region_end = mouse_pos;
			}

			if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
				m_region_end = mouse_pos;
			}

			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
				m_region_selection_active = false;
				m_region_selected = true;

				// Calculate region analysis
				ImVec2 roi_min = ImVec2(std::min(m_region_start.x, m_region_end.x),
							std::min(m_region_start.y, m_region_end.y));
				ImVec2 roi_max = ImVec2(std::max(m_region_start.x, m_region_end.x),
							std::max(m_region_start.y, m_region_end.y));

				int roi_width = std::max(1.0f, roi_max.x - roi_min.x);
				int roi_height = std::max(1.0f, roi_max.y - roi_min.y);

				// Get current frame data
				uint32_t *frame_data = (uint32_t *)malloc(image_size.x * image_size.y * 4);
				m_processed_textures[m_analysis_current_frame]->GetData(frame_data);

				// Analyze the selected region
				ImageAnalysis::AnalyzeRegion(frame_data, image_size.x, image_size.y, roi_min.x,
							     roi_min.y, roi_width, roi_height, m_region_histogram,
							     m_region_snr);

				free(frame_data);
			}
		}

		// Draw selection rectangle
		if (m_region_selection_active || m_region_selected) {
			ImDrawList *draw_list = ImGui::GetWindowDrawList();
			ImVec2 rect_min = ImVec2(image_pos.x + std::min(m_region_start.x, m_region_end.x),
						 image_pos.y + std::min(m_region_start.y, m_region_end.y));
			ImVec2 rect_max = ImVec2(image_pos.x + std::max(m_region_start.x, m_region_end.x),
						 image_pos.y + std::max(m_region_start.y, m_region_end.y));

			ImU32 color = m_region_selected ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 255, 0, 255);
			draw_list->AddRect(rect_min, rect_max, color, 0.0f, 0, 2.0f);
		}

		ImGui::EndChild();

		ImGui::EndTabItem();
	}
}

void ImageSet::DisplayFeatureTrackingTab() {
	static uint32_t *s_comparison_image = nullptr;
	static uint32_t s_ref_image_width = 0, s_ref_image_height = 0;
	if (ImGui::BeginTabItem("Feature Tracking")) {
		if (m_processed_textures.size() == 0) {
			ImGui::Text("No images loaded");
			ImGui::EndTabItem();
			return;
		}

		if (s_comparison_image == NULL ||
		    s_ref_image_width * s_ref_image_height !=
			m_processed_textures[0]->GetWidth() * m_processed_textures[0]->GetHeight()) {
			free(s_comparison_image);
			s_comparison_image = (uint32_t *)malloc(m_processed_textures[0]->GetWidth() *
								m_processed_textures[0]->GetHeight() * 4);
			s_ref_image_width = m_processed_textures[0]->GetWidth();
			s_ref_image_height = m_processed_textures[0]->GetHeight();
		}

		// Update point image if it isn't the same size as the ref
		// texture
		if (m_point_image == NULL ||
		    m_point_texture.GetWidth() * m_point_texture.GetHeight() !=
			m_processed_textures[0]->GetWidth() * m_processed_textures[0]->GetHeight()) {
			free(m_point_image);
			m_point_image = (uint32_t *)malloc(m_processed_textures[0]->GetWidth() *
							   m_processed_textures[0]->GetHeight() * 4);
		}

		m_processed_textures[0]->GetData(s_comparison_image);
		if (m_points.size() == 0 &&
		    memcmp(m_point_image, s_comparison_image,
			   m_processed_textures[0]->GetWidth() * m_processed_textures[0]->GetHeight() * 4) != 0) {
			memcpy(m_point_image, s_comparison_image,
			       m_processed_textures[0]->GetWidth() * m_processed_textures[0]->GetHeight() * 4);
			m_point_texture.Load(s_comparison_image, m_processed_textures[0]->GetWidth(),
					     m_processed_textures[0]->GetHeight());
		}

		ImGui::BeginChild("Controls", ImVec2(250, 0), true);
		static bool manualMode = false;
		ImGui::Text("Mode:");
		ImGui::RadioButton("Manual", (int *)&manualMode, true);
		ImGui::SameLine();
		ImGui::RadioButton("Auto", (int *)&manualMode, false);
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::Text("Manual mode allows you to select points to track "
				    "features. Auto mode will automatically detect "
				    "cracks and track their widths.\nWe suggest "
				    "cropping the infobar for automatic tracking.");
			ImGui::EndTooltip();
		}

		if (manualMode) {
			if (m_points.size() > 0 && ImGui::Button("Clear Selection"))
				m_points.clear();
			if (m_points.size() % 2 == 0 && m_points.size() > 0) {
				if (ImGui::Button("Track Features")) {
					std::vector<uint32_t *> frames;
					utils::GetDataFromTextures(frames, m_processed_textures[0]->GetWidth(),
								   m_processed_textures[0]->GetHeight(),
								   m_processed_textures);
					std::vector<std::vector<cv::Point2f>> tracked_points;
					m_manual_widths = FeatureTracker::TrackFeatures(
					    frames, m_points, tracked_points, m_processed_textures[0]->GetWidth(),
					    m_processed_textures[0]->GetHeight());
					memcpy(m_point_image, frames[0],
					       m_processed_textures[0]->GetWidth() *
						   m_processed_textures[0]->GetHeight() * 4);
					m_point_texture.Load(frames[0], m_processed_textures[0]->GetWidth(),
							     m_processed_textures[0]->GetHeight());
					utils::LoadDataIntoTexturesAndFree(m_processed_textures, frames,
									   m_processed_textures[0]->GetWidth(),
									   m_processed_textures[0]->GetHeight());
					m_last_points = m_points;
					m_last_tracked_points = tracked_points;
					m_points.clear();
				}
			} else {
				ImGui::Text("Select an even number of points");
			}
		} else {
			if (ImGui::Button("Track Widths")) {
				std::vector<uint32_t *> frames;
				utils::GetDataFromTextures(frames, m_processed_textures[0]->GetWidth(),
							   m_processed_textures[0]->GetHeight(), m_processed_textures);
				auto polygons = CrackDetector::DetectCracks(frames, m_processed_textures[0]->GetWidth(),
									    m_processed_textures[0]->GetHeight());
				m_widths = FeatureTracker::TrackCrackWidthProfiles(polygons); // Assuming m_widths is a
											      // member variable
				utils::LoadDataIntoTexturesAndFree(m_processed_textures, frames,
								   m_processed_textures[0]->GetWidth(),
								   m_processed_textures[0]->GetHeight());
			}
		}
		if (m_manual_widths.size() > 0 && manualMode) {
			if (ImGui::Button("Clear Widths")) {
				m_manual_widths.clear();
				m_last_points.clear();
				uint32_t *data =
				    (uint32_t *)malloc(m_textures[0]->GetWidth() * m_textures[0]->GetHeight() * 4);
				for (int i = 0; i < m_processed_textures.size(); i++) {
					m_textures[i]->GetData(data);
					m_processed_textures[i]->Load(data, m_processed_textures[i]->GetWidth(),
								      m_processed_textures[i]->GetHeight());
				}
				free(m_point_image);
				m_point_image = (uint32_t *)malloc(m_processed_textures[0]->GetWidth() *
								   m_processed_textures[0]->GetHeight() * 4);
				memcpy(m_point_image, data,
				       m_processed_textures[0]->GetWidth() * m_processed_textures[0]->GetHeight() * 4);
				m_point_texture.Load(m_point_image, m_processed_textures[0]->GetWidth(),
						     m_processed_textures[0]->GetHeight());
			}
			if (ImGui::Button("Save To")) {
				auto path = utils::SaveFileDialog(".", "Save Widths CSV", "csv");
				write_success = io::WriteCSV(path.c_str(), m_widths);
				if (!write_success) {
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error saving widths!");
				}
			}
			ImGui::Text("Manual Widths:");
			for (int i = 0; i < m_manual_widths.size(); i++) {
				ImGui::Text("Frame %d:", i);
				for (int j = 0; j < m_manual_widths[i].size(); j++) {
					if (j % 4 != 3)
						ImGui::SameLine();
					ImGui::Text("%.2f", m_manual_widths[i][j]);
				}
			}
		}
		if (m_widths.size() > 0 && !manualMode) {
			if (ImGui::Button("Clear Widths")) {
				m_widths.clear();
			}
			if (ImGui::Button("Save To")) {
				auto path = utils::SaveFileDialog(".", "Save Widths CSV", "csv");
				write_success = io::WriteCSV(path.c_str(), m_widths);
				if (!write_success) {
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error saving widths!");
				}
			}
		}
		ImGui::EndChild();

		// Image View
		ImGui::SameLine();
		ImGui::BeginChild("ImageView", ImVec2(0, 0), true);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::ImageButton("Processed Image", m_point_texture.GetID(),
				   ImVec2((float)m_point_texture.GetWidth(), (float)m_point_texture.GetHeight()));
		ImGui::PopStyleVar(2);

		if (manualMode && ImGui::IsItemActive() && ImGui::IsItemHovered()) {
			const auto now = std::chrono::system_clock::now();
			if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_time).count() > 250) {
				m_last_time = now;
				int coordX = (ImGui::GetMousePos().x - ImGui::GetItemRectMin().x);
				int coordY = (ImGui::GetMousePos().y - ImGui::GetItemRectMin().y);
				m_points.push_back(cv::Point2f(coordX, coordY));

				// where the user clicks, draw a red dot
				int size = 3;
				for (int i = coordX - size; i < coordX + size + 1; i++) {
					for (int j = coordY - size; j < coordY + size + 1; j++) {
						if (i < 0 || j < 0 || i >= m_processed_textures[0]->GetWidth() ||
						    j >= m_processed_textures[0]->GetHeight())
							continue;
						m_point_image[j * m_processed_textures[0]->GetWidth() + i] = 0xFFFF0000;
					}
				}
				m_point_texture.Load(m_point_image, m_processed_textures[0]->GetWidth(),
						     m_processed_textures[0]->GetHeight());
			}
		}
		ImGui::EndChild();

		ImGui::EndTabItem();
	}
}

void ImageSet::DisplayDeformationAnalysisTab() {
	static TileConfig compare_config;

	if (ImGui::BeginTabItem("Deformation Analysis")) {
		// left pane: settings + status
		ImGui::BeginChild("Controls", ImVec2(250, 0), true);

		// Check if processing is happening
		bool isProcessing = DeformationAnalysisInterface::IsProcessing();

		if (isProcessing) {
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Processing...");
			float progress = DeformationAnalysisInterface::GetProgress();
			ImGui::ProgressBar(progress, ImVec2(-1, 0), "");
		}

		ImGui::SeparatorText("View Settings");

		// View toggle
		ImGui::BeginDisabled(isProcessing);

		ImGui::Checkbox("Gallery View", &m_show_gallery_view);

		ImGui::EndDisabled();

		ImGui::SeparatorText("Tiling Configuration");

		ImGui::BeginDisabled(isProcessing);
		ImGui::Combo("Tile Type", (int *)&m_tile_config.type, "Cropped\0Blended\0\0");
		ImGui::SliderInt("Tile Size", &m_tile_config.tileSize, 156, 512);
		if (m_tile_config.type == TileType::Cropped) {
			ImGui::SliderInt("Center Size", &m_tile_config.centerSize, 16, 128);
			ImGui::Checkbox("Include Outside", &m_tile_config.includeOutside);
		} else if (m_tile_config.type == TileType::Blended) {
			ImGui::SliderInt("Overlap", &m_tile_config.overlap, 0, 128);
		}

		ImGui::EndDisabled();

		if (m_tile_config != compare_config)
			m_tile_need_refresh = true;

		compare_config = m_tile_config;

		// Add preview tile button
		static bool preview_tiles_open = false;
		ImGui::BeginDisabled(isProcessing);
		if (ImGui::Button("Preview Tiles")) {
			preview_tiles_open = true;
			// Generate preview tiles if they don't exist or if they need to be refreshed
			if (m_preview_tile_textures.empty() && !m_processed_textures.empty()) {
				utils::CreateTileTextures(m_preview_tile_textures, m_processed_textures[0],
							  m_tile_config);
			}
		}
		ImGui::EndDisabled();

		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Preview how the current image will be split into tiles\n"
					  "before running the full analysis. This helps you\n"
					  "adjust tile size and other parameters.");
		}

		// Use the common UI function to display the tile preview window
		auto refreshTiles = [this]() {
			if (!m_processed_textures.empty() && m_tile_need_refresh) {
				m_preview_tile_textures.clear();
				utils::UpdateTileTextures(m_preview_tile_textures, m_processed_textures[0],
							  m_tile_config);
				m_tile_need_refresh = false;
			}
		};

		ui::DisplayTilePreviewWindow("Tiled Image Preview", preview_tiles_open, m_preview_tile_textures,
					     refreshTiles);

		ImGui::Separator();

		// Run Analysis button
		ImGui::BeginDisabled(isProcessing || m_processed_textures.size() < 2);

		ImGui::SliderInt("Batch Size", &m_batch_size, 1, 32);
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Number of tiles to process in parallel.\nHigher values may improve "
					  "performance but require more memory.");
		}

		if (ImGui::Button("Run Analysis", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
			// gather frames
			utils::GetDataFromTextures(m_processing_frames, m_processed_textures[0]->GetWidth(),
						   m_processed_textures[0]->GetHeight(), m_processed_textures);

			// Clear previous results
			m_output_tiles.clear();
			m_output_tile_textures.clear();

			// Run the model asynchronously with the callback
			auto future = DeformationAnalysisInterface::RunModelBatchAsync(
			    m_processing_frames, m_processed_textures[0]->GetWidth(),
			    m_processed_textures[0]->GetHeight(), m_output_tiles, m_tile_config, m_batch_size,
			    [](bool b) {});

			// Store the future for polling in the next frame
			m_processing_future = std::make_shared<std::future<bool>>(std::move(future));

			m_output_tile_textures.clear();
		}
		ImGui::EndDisabled();

		if (!m_model_ok) {
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Model error!");
		}

		ImGui::Separator();
		ImGui::Text("Tiles: %d", (int)m_output_tiles.size());

		// For full image view, add a tile selector
		if (!m_show_gallery_view && !m_output_tile_textures.empty()) {
			ImGui::SliderInt("Current Tile", (int *)&m_current_tile_index, 0,
					 (int)m_output_tile_textures.size() - 1);
		}

		ImGui::EndChild();

		ImGui::SameLine();

		// right pane: either tile gallery or full image with overlay
		ImGui::BeginChild("TileView", ImVec2(0, 0));

		if (m_show_gallery_view) {
			// Gallery view
			ImGui::Text("Output Tiles");

			if (m_output_tile_textures.empty()) {
				ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "No tiles available. Run analysis first.");
			}

			// Textures are now loaded when processing completes in Display()

			const int cols = 4;
			for (int i = 0; i < m_output_tile_textures.size(); ++i) {
				if (i % cols != 0)
					ImGui::SameLine();
				ImGui::Image(m_output_tile_textures[i]->GetID(),
					     ImVec2((float)m_tile_config.tileSize, (float)m_tile_config.tileSize));

				// Make the tiles selectable for the full view
				if (ImGui::IsItemClicked()) {
					m_current_tile_index = i;
					m_show_gallery_view = false;
				}
			}
		} else {
			// Full image view with overlay of selected tile
			ImGui::Text("Full Image View");

			// Display the full image if available
			if (m_full_image_texture) {
				ImVec2 available_space = ImGui::GetContentRegionAvail();
				float scale_factor =
				    std::min(available_space.x / (float)m_full_image_texture->GetWidth(),
					     available_space.y / (float)m_full_image_texture->GetHeight());

				ImVec2 display_size(m_full_image_texture->GetWidth() * scale_factor,
						    m_full_image_texture->GetHeight() * scale_factor);

				ImGui::Image(m_full_image_texture->GetID(), display_size);

				// Indicate the position of the selected tile on the full image
				if (!m_output_tiles.empty() && m_current_tile_index < m_output_tiles.size()) {
					// Draw a rectangle overlay to indicate where the current tile is on the full
					// image This assumes tiles are stored in row-major order
					ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
					ImDrawList *draw_list = ImGui::GetWindowDrawList();

					// Calculate position of selected tile in full image coordinates
					const int full_width = m_full_image_texture->GetWidth();
					const int full_height = m_full_image_texture->GetHeight();

					// Get the rows and columns based on full image and tile size
					int cols_in_full = full_width / m_tile_config.tileSize;
					if (cols_in_full == 0)
						cols_in_full = 1;

					int tile_row = m_current_tile_index / cols_in_full;
					int tile_col = m_current_tile_index % cols_in_full;

					float rect_x_pos = cursor_pos.x - display_size.x +
							   (tile_col * m_tile_config.tileSize * scale_factor);
					float rect_y_pos = cursor_pos.y - display_size.y +
							   (tile_row * m_tile_config.tileSize * scale_factor);

					// Draw the rectangle
					draw_list->AddRect(ImVec2(rect_x_pos, rect_y_pos),
							   ImVec2(rect_x_pos + m_tile_config.tileSize * scale_factor,
								  rect_y_pos + m_tile_config.tileSize * scale_factor),
							   IM_COL32(255, 0, 0, 255), // Red
							   0.0f,		     // rounding
							   0,			     // flags
							   2.0f			     // thickness
					);
				}

				// Display the current selected tile in an inset
				if (!m_output_tile_textures.empty() &&
				    m_current_tile_index < m_output_tile_textures.size()) {
					// Draw the selected tile in a corner
					ImVec2 window_pos = ImGui::GetWindowPos();
					ImVec2 window_size = ImGui::GetWindowSize();

					// Position for the tile inset (bottom-right corner)
					float inset_size =
					    m_tile_config.tileSize / 1.5f; // slightly smaller than the original
					ImVec2 inset_pos(window_pos.x + window_size.x - inset_size -
							     20, // 20px padding from edge
							 window_pos.y + window_size.y - inset_size - 20);

					// Draw a background for the inset
					ImGui::GetWindowDrawList()->AddRectFilled(
					    inset_pos, ImVec2(inset_pos.x + inset_size, inset_pos.y + inset_size),
					    IM_COL32(40, 40, 40, 200) // Dark background with some transparency
					);

					// Draw the tile
					ImGui::GetWindowDrawList()->AddImage(
					    m_output_tile_textures[m_current_tile_index]->GetID(), inset_pos,
					    ImVec2(inset_pos.x + inset_size, inset_pos.y + inset_size));
				}
			} else {
				ImGui::TextColored(ImVec4(1, 0.5f, 0, 1),
						   "No full image available. Run analysis first.");
			}
		}

		ImGui::EndChild();

		ImGui::EndTabItem();
	}
}
