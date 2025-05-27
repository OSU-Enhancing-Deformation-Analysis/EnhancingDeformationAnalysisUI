struct GLFWwindow;

void ImGuiInit(GLFWwindow *window, bool set_colors = true,
	       bool set_fonts = true);
void ImGuiShutdown();

void ImGuiBeginFrame();

void ImGuiEndFrame();
