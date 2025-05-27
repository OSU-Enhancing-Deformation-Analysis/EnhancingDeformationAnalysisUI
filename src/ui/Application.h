#pragma once

#include <memory>
#include <vector>

#include <imgui.h>

#include <ui/ImageSet.h>

struct GLFWwindow;

class Application {
      public:
	Application();
	~Application();

	// Initialize the application
	bool Initialize();

	// Run the application main loop
	void Run();

	// Clean up resources
	void Shutdown();

      private:
	// Initialize GLFW and OpenGL
	bool InitializeGLFW();

	// Initialize ImGui
	void InitializeImGui();

	// Render the main UI
	void RenderUI();

	// Render the welcome screen
	void RenderWelcomeScreen();

	// Render the folder selector UI
	void RenderFolderSelector();

	// Process and display all image sets
	void RenderImageSets();

	// Window handle
	GLFWwindow *m_window;

	// Image sets
	std::vector<std::unique_ptr<ImageSet>> m_imageSets;

	// UI state
	bool m_showWelcome;
	bool m_assetsFound;

	// Docking state
	ImGuiID m_dockspaceID;
};
