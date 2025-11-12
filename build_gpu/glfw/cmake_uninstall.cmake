
if (NOT EXISTS "/nfs/stak/users/gemmak/CS461/EnhancingDeformationAnalysisUI/build_gpu/glfw/install_manifest.txt")
    message(FATAL_ERROR "Cannot find install manifest: \"/nfs/stak/users/gemmak/CS461/EnhancingDeformationAnalysisUI/build_gpu/glfw/install_manifest.txt\"")
endif()

file(READ "/nfs/stak/users/gemmak/CS461/EnhancingDeformationAnalysisUI/build_gpu/glfw/install_manifest.txt" files)
string(REGEX REPLACE "\n" ";" files "${files}")

foreach (file ${files})
  message(STATUS "Uninstalling \"$ENV{DESTDIR}${file}\"")
  if (EXISTS "$ENV{DESTDIR}${file}")
    exec_program("/nfs/stak/a1/rhel5apps/spack/opt-el8/spack/linux-sandybridge/cmake-3.31.8-63trq3jyfruewhi5wdjkab43vuk4rxla/bin/cmake" ARGS "-E remove \"$ENV{DESTDIR}${file}\""
                 OUTPUT_VARIABLE rm_out
                 RETURN_VALUE rm_retval)
    if (NOT "${rm_retval}" STREQUAL 0)
      MESSAGE(FATAL_ERROR "Problem when removing \"$ENV{DESTDIR}${file}\"")
    endif()
  elseif (IS_SYMLINK "$ENV{DESTDIR}${file}")
    EXEC_PROGRAM("/nfs/stak/a1/rhel5apps/spack/opt-el8/spack/linux-sandybridge/cmake-3.31.8-63trq3jyfruewhi5wdjkab43vuk4rxla/bin/cmake" ARGS "-E remove \"$ENV{DESTDIR}${file}\""
                 OUTPUT_VARIABLE rm_out
                 RETURN_VALUE rm_retval)
    if (NOT "${rm_retval}" STREQUAL 0)
      message(FATAL_ERROR "Problem when removing symlink \"$ENV{DESTDIR}${file}\"")
    endif()
  else()
    message(STATUS "File \"$ENV{DESTDIR}${file}\" does not exist.")
  endif()
endforeach()

