#include <ui/Application.h>

// glad must be above glfw, includes opengl header itself
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <ImGuiImpl.h>
#include <utils.h>

Application::Application()
    : m_window(nullptr), m_showWelcome(true), m_assetsFound(false),
      m_dockspaceID(0) {}

Application::~Application() { Shutdown(); }

bool Application::Initialize() {
#ifndef UI_RELEASE
	std::filesystem::current_path("../");
#endif // UI_RELEASE

	// Verify assets folder existence
	m_assetsFound = std::filesystem::exists("assets");
	if (!m_assetsFound) {
		fprintf(stderr,
			"ERROR: Assets folder not found! The folder is "
			"required for this program to function correctly!\n");
	}

	// Initialize GLFW and OpenGL
	if (!InitializeGLFW()) {
		return false;
	}

	// Initialize ImGui
	InitializeImGui();

	return true;
}

bool Application::InitializeGLFW() {
	if (!glfwInit()) {
		printf("Failed to initialize GLFW\n");
		return false;
	}

#ifdef __APPLE__
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // for macOS
#endif

#ifdef UI_RELEASE
	m_window = glfwCreateWindow(
	    1400, 1050, "Enhancing Deformation Analysis UI", NULL, NULL);
#else
	m_window = glfwCreateWindow(1400, 1050,
				    "Enhancing Deformation Analysis UI (DEBUG)",
				    NULL, NULL);
#endif

	glfwMakeContextCurrent(m_window);
	int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	if (!status) {
		printf("Failed to initialize OpenGL context\n");
		printf("This is likely due to an issue with the graphics "
		       "driver/card\n");
		printf("If you do not have a supported display adapter (e.g., "
		       "if you are using a VM), use the command line interface "
		       "instead\n");
		return false;
	}

	// Set GLFW error callback
	glfwSetErrorCallback([](int error, const char *description) {
		fprintf(stderr, "GLFW Error %d: %s\n", error, description);
	});

	// Enable vsync to limit frame rate
	glfwSwapInterval(1);

	return true;
}

void Application::InitializeImGui() {
	constexpr bool set_colors = true;
	// Initialize ImGui context and install GLFW callbacks
	ImGuiInit(m_window, set_colors, m_assetsFound);

	// Default to dark theme if no custom theme
	if (!set_colors) {
		ImGui::StyleColorsDark();
	}

	// Enable docking
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

void Application::Run() {
	while (!glfwWindowShouldClose(m_window)) {
		// Set the default background and clear the screen
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Check for events
		glfwPollEvents();

		// Begin the ImGui frame
		ImGuiBeginFrame();

		// Render the dockspace for all other windows
		m_dockspaceID = ImGui::DockSpaceOverViewport();

		// Show demo window in debug builds
#ifndef UI_RELEASE
		ImGui::ShowDemoWindow();
#endif

		// Render the main UI
		RenderUI();

		// End the ImGui frame
		ImGuiEndFrame();

		glfwSwapBuffers(m_window);
	}
}

void Application::RenderUI() {
	ImGui::SetNextWindowDockID(m_dockspaceID, ImGuiCond_FirstUseEver);

	// Create window for folder selector and welcome screen
	ImGui::Begin("Image Folder Selector");

	// Show welcome screen if needed
	if (m_showWelcome) {
		RenderWelcomeScreen();
	}

	// Show folder selector
	RenderFolderSelector();

	// Show asset warning if needed
	if (!m_assetsFound) {
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
				   "WARNING: Assets folder not found!");
		ImGui::TextWrapped(
		    "Please place the assets folder in the same directory as "
		    "the executable. The assets folder is required for the "
		    "program to function correctly.");
	}

	ImGui::End();

	// Render all image sets
	RenderImageSets();
}

void Application::RenderWelcomeScreen() {
	// Styled title
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 1.0f, 1.0f));
	ImGui::SetWindowFontScale(1.5f);
	ImGui::SetCursorPosX(
	    (ImGui::GetWindowWidth() -
	     ImGui::CalcTextSize("Deformation Analysis UI").x) *
	    0.5f);
	ImGui::Text("Deformation Analysis UI");
	ImGui::SetWindowFontScale(1.0f);
	ImGui::PopStyleColor();

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Description
	ImGui::TextWrapped("This application helps analyze deformation in "
			   "microscopy images using AI-powered tools.");
	ImGui::Spacing();

	// Key features
	ImGui::TextColored(ImVec4(0.28f, 0.56f, 1.0f, 1.0f), "Key Features:");
	ImGui::Bullet();
	ImGui::TextWrapped("Deformation tracking and analysis");
	ImGui::Bullet();
	ImGui::TextWrapped("Image stabilization");
	ImGui::Bullet();
	ImGui::TextWrapped("Feature tracking across image sequences");
	ImGui::Bullet();
	ImGui::TextWrapped("AI-powered crack detection");

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Getting started
	ImGui::TextColored(ImVec4(0.28f, 0.56f, 1.0f, 1.0f),
			   "Getting Started:");
	ImGui::TextWrapped("1. Click the 'Select Folder' button below to load "
			   "your TIFF images");
	ImGui::TextWrapped("2. Each image set will open in a new tab");
	ImGui::TextWrapped("3. Use the tabs within each image set for "
			   "stabilization, preprocessing, and analysis");

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextColored(ImVec4(0.28f, 0.56f, 1.0f, 1.0f),
			   "This build features:");
#ifdef UI_INCLUDE_PYTORCH
	ImGui::Bullet();
	ImGui::TextWrapped("PyTorch support for AI models");
#endif
#ifdef UI_INCLUDE_TENSORFLOW
	ImGui::Bullet();
	ImGui::TextWrapped("TensorFlow support for AI models");
#endif
#ifndef UI_RELEASE
	ImGui::Bullet();
	ImGui::TextColored(ImVec4(0.8f, 0.5f, 0.5f, 1.0f),
			   "Debug build with additional features!");
#endif

	ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.25f);
	if (ImGui::Button("Hide Welcome Screen",
			  ImVec2(ImGui::GetWindowWidth() * 0.5f, 0))) {
		m_showWelcome = false;
	}

	ImGui::SetCursorPosX(
	    (ImGui::GetWindowWidth() - ImGui::GetItemRectSize().x) * 0.5f);
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
}

void Application::RenderFolderSelector() {
#ifdef __APPLE__
	ImGui::InputTextWithHint("##image_folder", "Enter image folder path",
				 nullptr, 0);
	if (ImGui::Button("Load Images")) {
		const char *folder_path =
		    ImGui::GetInputTextState("##image_folder")->Text;
		if (std::filesystem::is_directory(folder_path)) {
			m_imageSets.emplace_back(
			    std::make_unique<ImageSet>(folder_path));
		}
	}
#else
	// Make the folder selector button more prominent
	ImGui::PushStyleColor(ImGuiCol_Button,
			      ImVec4(0.28f, 0.56f, 1.0f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
			      ImVec4(0.28f, 0.56f, 1.0f, 0.9f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,
			      ImVec4(0.28f, 0.56f, 1.0f, 1.0f));

	// Center the button horizontally
	float buttonWidth = 150.0f;
	ImGui::SetCursorPosX((ImGui::GetWindowWidth() - buttonWidth) * 0.5f);

	if (ImGui::Button("Select Folder", ImVec2(buttonWidth, 35))) {
		std::string folder_path =
		    utils::OpenFileDialog(".", "Choose a Folder to Load", true);
		if (!folder_path.empty() &&
		    std::filesystem::is_directory(folder_path) &&
		    utils::DirectoryContainsTiff(folder_path)) {
			m_imageSets.emplace_back(
			    std::make_unique<ImageSet>(folder_path));
		}
	}

	ImGui::PopStyleColor(3);
#endif
}

void Application::RenderImageSets() {
	// Process all image sets
	for (auto it = m_imageSets.begin(); it != m_imageSets.end();) {
		// Check if the image set is closed
		if ((*it)->Closed()) {
			// Remove closed image set
			it = m_imageSets.erase(it);
		} else {
			// Display the image set
			ImGui::SetNextWindowDockID(m_dockspaceID,
						   ImGuiCond_FirstUseEver);
			(*it)->Display();
			++it;
		}
	}
}

void Application::Shutdown() {
	// Clean up image sets
	m_imageSets.clear();

	// Destroy GLFW window and terminate GLFW
	if (m_window) {
		glfwDestroyWindow(m_window);
		m_window = nullptr;
	}

	glfwTerminate();
}
