# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/nfs/stak/users/gemmak/CS461/EnhancingDeformationAnalysisUI/libs/libtiff")
  file(MAKE_DIRECTORY "/nfs/stak/users/gemmak/CS461/EnhancingDeformationAnalysisUI/libs/libtiff")
endif()
file(MAKE_DIRECTORY
  "/nfs/stak/users/gemmak/CS461/EnhancingDeformationAnalysisUI/build_gpu/libtiff"
  "/nfs/stak/users/gemmak/CS461/EnhancingDeformationAnalysisUI/build_gpu/_deps/libtiff-subbuild/libtiff-populate-prefix"
  "/nfs/stak/users/gemmak/CS461/EnhancingDeformationAnalysisUI/build_gpu/_deps/libtiff-subbuild/libtiff-populate-prefix/tmp"
  "/nfs/stak/users/gemmak/CS461/EnhancingDeformationAnalysisUI/build_gpu/_deps/libtiff-subbuild/libtiff-populate-prefix/src/libtiff-populate-stamp"
  "/nfs/stak/users/gemmak/CS461/EnhancingDeformationAnalysisUI/build_gpu/_deps/libtiff-subbuild/libtiff-populate-prefix/src"
  "/nfs/stak/users/gemmak/CS461/EnhancingDeformationAnalysisUI/build_gpu/_deps/libtiff-subbuild/libtiff-populate-prefix/src/libtiff-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/nfs/stak/users/gemmak/CS461/EnhancingDeformationAnalysisUI/build_gpu/_deps/libtiff-subbuild/libtiff-populate-prefix/src/libtiff-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/nfs/stak/users/gemmak/CS461/EnhancingDeformationAnalysisUI/build_gpu/_deps/libtiff-subbuild/libtiff-populate-prefix/src/libtiff-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
