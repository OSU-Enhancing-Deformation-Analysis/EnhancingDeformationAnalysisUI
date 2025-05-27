#include <ui/Application.h>

#include <cli.hpp>

int main(int argc, char **argv) {
	// If there are any args, run the CLI instead
	if (argc > 1) {
		cli::run(argc, argv);
		return 0;
	}

	// Create and initialize the application
	Application app;
	if (!app.Initialize()) {
		return -1;
	}

	// Run the application
	app.Run();

	return 0;
}
