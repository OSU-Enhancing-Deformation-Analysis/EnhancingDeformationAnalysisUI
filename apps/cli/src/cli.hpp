#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace cli {
// entry point for the command-line interface
void run(int argc, char **argv);

// convenience entry for tests
int run_argv(const std::vector<std::string> &args);
} // namespace cli