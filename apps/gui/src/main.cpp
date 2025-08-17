#include <Application.h>

int main(int argc, char **argv) {
	// Create and initialize the application
	Application app;
	if (!app.Initialize()) {
		return -1;
	}

	// Run the application
	app.Run();

	return 0;
}

// this is required for Windows applications without a console window
#ifdef _WIN32
#include <windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	return main(__argc, __argv);
}
#endif