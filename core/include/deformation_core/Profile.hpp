#pragma once

#include <chrono>
#include <cstdio>

class Profiler {
      public:
	Profiler(const char *name = "'empty'")
	    : name(name), start_time(std::chrono::high_resolution_clock::now()) {}

	~Profiler() { Stop(); }

	void Stop() {
		if (stopped)
			return;
		stopped = true;
		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration =
		    std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
		printf("Profiler: %s took %zu ms\n", name, (size_t)duration);
	}

      private:
	const char *name;
	std::chrono::high_resolution_clock::time_point start_time;
	bool stopped = false;
};

#ifdef EDA_PROFILE
#define PROFILE_FUNCTION(name) Profiler profiler_##name(__FUNCTION__);
#define PROFILE_SCOPE(name) Profiler profiler_##name(#name);
#else
#define PROFILE_FUNCTION(name)
#define PROFILE_SCOPE(name)
#endif
