cmake_minimum_required(VERSION 3.5)

project(EnhancingDeformationAnalysisUI VERSION 0.1
	DESCRIPTION "UI for the Enhancing Deformation Analysis capstone"
	LANGUAGES CXX C)

# Enable compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs/cppflow/cmake/modules)

# Set the source directory
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Find all .cpp files in the source directory and its subdirectories
file(GLOB_RECURSE SOURCES "${SRC_DIR}/**.cpp" "${SRC_DIR}/**.h")

file(GLOB IMGUI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui/*.cpp")

if (!WIN32)
	SET(GLFW_BUILD_X11 ON CACHE BOOL "Build for X11")
	SET(GLFW_BUILD_WAYLAND ON CACHE BOOL "Build for Wayland")
endif()

SET(BUILD_EXAMPLES OFF CACHE BOOL "cppflow lib only")
SET(tiff-tools OFF CACHE BOOL "libtiff lib only")
SET(tiff-tests OFF CACHE BOOL "libtiff lib only")
SET(tiff-contrib OFF CACHE BOOL "libtiff lib only")
SET(tiff-docs OFF CACHE BOOL "libtiff lib only")
SET(tiff-install OFF CACHE BOOL "libtiff lib only")

find_package(OpenCV REQUIRED)

add_subdirectory(libs/glfw)
add_subdirectory(libs/libtiff)
add_subdirectory(libs/cppflow)

# Don't let glfw build examples/tests/docs or install
SET(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
SET(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "GLFW lib only")
SET(GLFW_BUILD_TESTS OFF CACHE BOOL "GLFW lib only")
SET(GLFW_INSTALL OFF CACHE BOOL "GLFW lib only")

add_executable(EnhancingDeformationAnalysisUI ${SOURCES} libs/glad/src/glad.c ${IMGUI_SOURCES})

# For imgui to not try and load OpenGL itself
target_compile_definitions(EnhancingDeformationAnalysisUI PRIVATE IMGUI_IMPL_OPENGL_LOADER_CUSTOM)

# Link to glfw
target_link_libraries(EnhancingDeformationAnalysisUI PRIVATE glfw)

target_link_libraries(EnhancingDeformationAnalysisUI PRIVATE tiff)

target_link_libraries(EnhancingDeformationAnalysisUI PRIVATE cppflow)

target_link_libraries(EnhancingDeformationAnalysisUI PRIVATE ${OpenCV_LIBS})

# Include directories
target_include_directories(EnhancingDeformationAnalysisUI PRIVATE src/ libs/ libs/glad/include/ libs/imgui/)

# Include OpenCV
target_include_directories(EnhancingDeformationAnalysisUI PRIVATE ${OpenCV_INCLUDE_DIRS})

target_include_directories(EnhancingDeformationAnalysisUI PRIVATE libs/cppflow/include)

target_include_directories(EnhancingDeformationAnalysisUI PRIVATE libs/gif-h)

# Set C++ standard to 20
target_compile_features(EnhancingDeformationAnalysisUI PRIVATE cxx_std_20)

if (UI_RELEASE)
	target_compile_definitions(EnhancingDeformationAnalysisUI PRIVATE UI_RELEASE)
	target_compile_options(EnhancingDeformationAnalysisUI PRIVATE -Wall)

# Windows doesn't like setting the optimization level through CMake
# TODO: investigate this
if (!WIN32)
	target_compile_options(EnhancingDeformationAnalysisUI PRIVATE -O3)
endif() # !WIN32
else()
if (!WIN32)
	target_compile_options(EnhancingDeformationAnalysisUI PRIVATE -O)
endif() # !WIN32
	target_compile_options(EnhancingDeformationAnalysisUI PRIVATE -g)
endif() # UI_RELEASE
