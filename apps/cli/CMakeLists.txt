# apps/cli/CMakeLists.txt
set(CLI_SRC
  src/main_cli.cpp
  src/cli.cpp
  src/utils.cpp)             # existing CLI helpers migrated from old src/cli.cpp

add_executable(eda-cli ${CLI_SRC})

target_compile_features(eda-cli PRIVATE cxx_std_20)

# only what cli needs
target_link_libraries(eda-cli
  PRIVATE deformation_core
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
