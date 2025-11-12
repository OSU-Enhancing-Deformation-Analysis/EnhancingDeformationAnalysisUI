# apps/gui/CMakeLists.txt
file(GLOB GUI_SRC CONFIGURE_DEPENDS
  "src/*.cpp"
  "../../libs/glad/src/*.c"
  "../../libs/imgui/*.cpp")

# gui should be windowed w/ no cmd on windows
if (WIN32)
  add_executable(eda-gui WIN32 ${GUI_SRC})
else()
  add_executable(eda-gui ${GUI_SRC})
endif()

target_compile_features(eda-gui PRIVATE cxx_std_20)

# bring in vendored deps only here (not in core, not in cli)
if (EDA_USE_VENDORED_DEPS)
  add_subdirectory(${EDA_ROOT}/libs/glfw   ${CMAKE_BINARY_DIR}/glfw)
  #add_subdirectory(${EDA_ROOT}/libs/imgui  ${CMAKE_BINARY_DIR}/imgui)
  # glad is just a source file; include its headers and compile its .c in GUI_SRC
  target_include_directories(eda-gui PRIVATE ${EDA_ROOT}/libs/glad/include)
  # gif-h is header-only
  target_include_directories(eda-gui PRIVATE ${EDA_ROOT}/libs/gif-h)
else()
  find_package(glfw3 REQUIRED)
endif()

# compile flag so imgui uses our glad
target_compile_definitions(eda-gui PRIVATE IMGUI_IMPL_OPENGL_LOADER_CUSTOM)

target_link_libraries(eda-gui
  PRIVATE deformation_core
          glfw
          ${OpenCV_LIBS})
 
# The added code



include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
    #include <format>
    int main() { std::format(\"{}\", 1); }
" HAS_STD_FORMAT)

if (NOT HAS_STD_FORMAT)
    message(WARNING "⚠️ <format> not available; falling back to fmtlib for GUI")
    target_link_libraries(eda-gui PRIVATE fmt::fmt)
    target_include_directories(eda-gui PRIVATE ${CMAKE_SOURCE_DIR}/libs/fmt/include)
    target_compile_definitions(eda-gui PRIVATE USE_FMT_FALLBACK=1)
endif()

# --- Manually link CUPTI for CUDA 11.8 on OSU HPC ---
set(CUDA_TOOLKIT_ROOT_DIR "/usr/local/apps/cuda/11.8")

set(CUPTI_LIB_PATH "${CUDA_TOOLKIT_ROOT_DIR}/extras/CUPTI/lib64/libcupti.so")
set(CUPTI_INCLUDE_PATH "${CUDA_TOOLKIT_ROOT_DIR}/extras/CUPTI/include")

if (EXISTS "${CUPTI_LIB_PATH}")
    message(STATUS "✅ Found CUPTI at: ${CUPTI_LIB_PATH}")
    target_link_libraries(eda-gui PRIVATE
        ${CUPTI_LIB_PATH}
        ${CUDA_TOOLKIT_ROOT_DIR}/lib64/libcudart.so
    )
    target_include_directories(eda-gui PRIVATE ${CUPTI_INCLUDE_PATH})
else()
    message(WARNING "⚠ CUPTI not found at ${CUPTI_LIB_PATH} — profiler symbols may remain unresolved.")
endif()
# --- End CUPTI section ---

# The end of added code

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
  target_link_libraries(eda-gui PRIVATE tiff)
else()
  find_package(TIFF REQUIRED)
  target_link_libraries(eda-gui PRIVATE TIFF::TIFF)  # imported target from find_package
endif()

# helpful defines for UI titlepaths, etc.
if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_definitions(eda-gui PRIVATE EDA_GUI_RELEASE)
else()
  target_compile_definitions(eda-gui PRIVATE EDA_GUI_DEBUG)
endif()

target_include_directories(eda-gui PRIVATE
  ${EDA_ROOT}/apps/gui/src
  ${EDA_ROOT}/libs/imgui
  ${EDA_ROOT}/libs/glad/include
  ${EDA_ROOT}/libs/stb_image
  ${EDA_ROOT}/libs/libtiff/libtiff
  ${EDA_ROOT}/libs/gif-h/
  ${EDA_ROOT}/libs)
target_include_directories(eda-gui PRIVATE ${CMAKE_BINARY_DIR}/libtiff/libtiff)

# windows: windowed subsystem (no console popping up)
if (WIN32)
  target_link_options(eda-gui PRIVATE "/SUBSYSTEM:WINDOWS")
endif()
