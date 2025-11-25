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

# macos: add objc++ file and cocoa framework
if(APPLE)
  # add the .mm impl for the file dialog
  target_sources(eda-gui PRIVATE
    ${EDA_ROOT}/apps/gui/src/utils.mm
  )

  # force objc++ mode on that file (clang usually infers it from .mm but
  # some older cmakes get salty, so belt and suspenders)
  set_source_files_properties(
    ${EDA_ROOT}/apps/gui/src/utils.mm
    PROPERTIES LANGUAGE OBJCXX
  )

  # link cocoa so NSOpenPanel works
  target_link_libraries(eda-gui PRIVATE "-framework Cocoa")
endif()

# windows: windowed subsystem (no console popping up)
if (WIN32)
  target_link_options(eda-gui PRIVATE "/SUBSYSTEM:WINDOWS")
endif()
