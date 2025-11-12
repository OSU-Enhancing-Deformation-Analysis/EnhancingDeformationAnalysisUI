# apps/cli/CMakeLists.txt
set(CLI_SRC
  src/main_cli.cpp
  src/cli.cpp
  src/utils.cpp)             # existing CLI helpers migrated from old src/cli.cpp

# New stuff
include(CheckCXXSourceCompiles)

# Test if <format> is supported
check_cxx_source_compiles("
  #include <format>
  int main() { auto s = std::format(\"{}\", 1); return 0; }
" HAS_STD_FORMAT)

if (HAS_STD_FORMAT)
  message(STATUS "Using std::format from C++20")
  add_compile_definitions(USE_STD_FORMAT)
else()
  message(WARNING "⚠️ <format> not found; falling back to fmtlib")
  # add_subdirectory(${PROJECT_SOURCE_DIR}/libs/fmt ${CMAKE_BINARY_DIR}/libs/fmt)
  add_compile_definitions(USE_FMTLIB)
endif()
# End of new stuff

add_executable(eda-cli ${CLI_SRC})
target_compile_features(eda-cli PRIVATE cxx_std_20)
find_package(Threads REQUIRED)
# ---- Link everything cleanly ----
if (TARGET fmt::fmt)
    target_link_libraries(eda-cli PRIVATE deformation_core ${OpenCV_LIBS} fmt::fmt Threads::Threads)
elseif (TARGET fmt)
    target_link_libraries(eda-cli PRIVATE deformation_core ${OpenCV_LIBS} fmt Threads::Threads)
else()
    message(WARNING "⚠️ fmt not found at link time; building may fail.")
    target_link_libraries(eda-cli PRIVATE deformation_core ${OpenCV_LIBS} Threads::Threads)
endif()


find_package(CUDA REQUIRED)

# --- Optional CUPTI profiling support ---
if (EXISTS "/usr/local/apps/cuda/11.8/extras/CUPTI/include")
    set(CUPTI_INCLUDE_DIR "/usr/local/apps/cuda/11.8/extras/CUPTI/include")
    set(CUPTI_LIBRARY "/usr/local/apps/cuda/11.8/extras/CUPTI/lib64/libcupti.so")
    message(STATUS "✔ Found CUPTI: ${CUPTI_LIBRARY}")
    target_include_directories(eda-cli PRIVATE ${CUPTI_INCLUDE_DIR})
    target_link_libraries(eda-cli PRIVATE ${CUPTI_LIBRARY})
else()
    message(WARNING "⚠ CUPTI not found; profiling will be disabled.")
endif()

target_link_libraries(eda-cli
    PRIVATE
        fmt::fmt
        ${TORCH_LIBRARIES}
        ${CUDA_LIBRARIES}
        ${CUDA_CUDART_LIBRARY}
)

# Link CUPTI only if it exists
if (CUPTI_LIBRARY)
    target_link_libraries(eda-cli PRIVATE ${CUPTI_LIBRARY})
    message(STATUS "✔ Found CUPTI: ${CUPTI_LIBRARY}")
else()
    message(WARNING "⚠ CUPTI not found; profiling will be disabled.")
endif()


# don't delete this

# only what cli needs
# target_link_libraries(eda-cli
# PRIVATE deformation_core
#        ${OpenCV_LIBS})

# vendored libtiff
if (EDA_USE_VENDORED_DEPS)
  include(FetchContent)
  FetchContent_Declare(
    libtiff
    SOURCE_DIR ${EDA_ROOT}/libs/libtiff
    BINARY_DIR ${CMAKE_BINARY_DIR}/libtiff
    SYSTEM
  )
  FetchContent_MakeAvailable(libtiff)
  target_link_libraries(eda-cli PRIVATE tiff)        # or tiffxx if you use the c++ api
else()
  find_package(TIFF REQUIRED)
  target_link_libraries(eda-cli PRIVATE TIFF::TIFF)  # imported target from find_package
endif()

target_include_directories(eda-cli PRIVATE
  ${EDA_ROOT}/apps/cli/src
  ${EDA_ROOT}/libs/libtiff/libtiff)
target_include_directories(eda-cli PRIVATE ${CMAKE_BINARY_DIR}/libtiff/libtiff)

# windows: console subsystem (proper stdout/stderr)
if (WIN32)
  target_link_options(eda-cli PRIVATE "/SUBSYSTEM:CONSOLE")
endif()
