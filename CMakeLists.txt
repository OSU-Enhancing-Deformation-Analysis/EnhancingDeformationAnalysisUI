cmake_minimum_required(VERSION 3.20)

project(eda
  LANGUAGES C CXX
  DESCRIPTION "enhancing deformation analysis ¿ core + gui + cli")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# feature switches (defaults sane)
option(EDA_WITH_PYTORCH      "build core with pytorch backend" ON)
option(EDA_WITH_TENSORFLOW   "build core with tensorflow/cppflow backend" OFF)
option(EDA_PROFILE           "enable lightweight profiling macros" OFF)
option(EDA_BUILD_GUI         "build imgui gui app" ON)
option(EDA_BUILD_CLI         "build cli app" ON)
option(EDA_USE_VENDORED_DEPS "use libs/* instead of a package manager" ON)

# --- Build vendored fmt manually (force compiled mode) ---
set(FMT_SRC
    ${CMAKE_SOURCE_DIR}/libs/fmt/src/format.cc
    ${CMAKE_SOURCE_DIR}/libs/fmt/src/os.cc
)
add_library(fmt STATIC ${FMT_SRC})
target_include_directories(fmt PUBLIC ${CMAKE_SOURCE_DIR}/libs/fmt/include)
# Prevent header-only mode
# target_compile_definitions(fmt PRIVATE FMT_EXPORT PUBLIC FMT_SHARED)
set_target_properties(fmt PROPERTIES POSITION_INDEPENDENT_CODE ON)
add_library(fmt::fmt ALIAS fmt)

message(STATUS "¿ Vendored fmt compiled library configured.")

# opencv is required by core
find_package(OpenCV REQUIRED)

# expose common vars to subdirs
set(EDA_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(core)
if (EDA_BUILD_CLI)
  add_subdirectory(apps/cli)
endif()
if (EDA_BUILD_GUI)
  add_subdirectory(apps/gui)
endif()
