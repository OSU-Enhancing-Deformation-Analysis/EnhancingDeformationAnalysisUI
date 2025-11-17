# core/CMakeLists.txt
file(GLOB CORE_SRC CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(deformation_core STATIC ${CORE_SRC})

target_compile_features(deformation_core PUBLIC cxx_std_20)
target_include_directories(deformation_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include         # <deformation_core/...>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

# core depends on opencv (but NOT on imgui/glfw/glad/tiff)
#target_link_libraries(deformation_core
# PUBLIC ${OpenCV_LIBS})

# Ensure OpenCV headers are visible to this target
#target_include_directories(deformation_core
#	PUBLIC ${OpenCV_INCLUDE_DIRS})

set(OpenCV_DIR "$ENV{OpenCV_DIR}" CACHE PATH "Path to OpenCVConfig.cmake")
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui)

if(OpenCV_FOUND)
    message(STATUS "✅ Found OpenCV ${OpenCV_VERSION}")
    message(STATUS "    Include dirs: ${OpenCV_INCLUDE_DIRS}")
    message(STATUS "    Libraries: ${OpenCV_LIBS}")
else()
    message(FATAL_ERROR "❌ OpenCV not found! Please verify OpenCV_DIR.")
endif()

target_include_directories(deformation_core PUBLIC ${OpenCV_INCLUDE_DIRS})
target_link_libraries(deformation_core PUBLIC ${OpenCV_LIBS})


# optional backends
if (EDA_WITH_PYTORCH)
  set(CMAKE_CUDA_ARCHITECTURES "75")
  find_package(Torch REQUIRED)
  target_link_libraries(deformation_core PUBLIC ${TORCH_LIBRARIES})
  target_compile_definitions(deformation_core PUBLIC EDA_WITH_PYTORCH)
  target_include_directories(deformation_core PUBLIC ${TORCH_INCLUDE_DIRS})
endif()

if (EDA_WITH_TENSORFLOW)
  if (EDA_USE_VENDORED_DEPS)
    add_subdirectory(${EDA_ROOT}/libs/cppflow ${CMAKE_BINARY_DIR}/cppflow)
    target_link_libraries(deformation_core PUBLIC cppflow)
    target_include_directories(deformation_core PUBLIC ${EDA_ROOT}/libs/cppflow/include)
  else()
    message(FATAL_ERROR "non-vendored tf/cppflow not wired up; set EDA_USE_VENDORED_DEPS=ON or turn off EDA_WITH_TENSORFLOW")
  endif()
  target_compile_definitions(deformation_core PUBLIC EDA_WITH_TENSORFLOW)
endif()

if (EDA_PROFILE)
  target_compile_definitions(deformation_core PUBLIC EDA_PROFILE)
endif()
