# Copyright (c) 2025 acrion innovations GmbH
# Authors: Stefan Zipproth, s.zipproth@acrion.ch
#
# This file is part of acrion image tools, see https://github.com/acrion/image-tools
#
# acrion image tools is offered under a commercial and under the AGPL license.
# For commercial licensing, contact us at https://acrion.ch/sales. For AGPL licensing, see below.
#
# AGPL licensing:
# acrion image tools is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# acrion image tools is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with acrion image tools. If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.25.2)
cmake_policy(VERSION 3.25.2)

include(FetchContent)
FetchContent_Declare(
    acrion_cmake
    GIT_REPOSITORY https://github.com/acrion/cmake.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(acrion_cmake)
if (NOT acrion_cmake_POPULATED)
    FetchContent_MakeAvailable(acrion_cmake)
endif ()

if (NOT TARGET acrion_image)
    FetchContent_Declare(
        acrion_image
        # GIT_REPOSITORY https://github.com/acrion/image.git
        GIT_REPOSITORY git@github.com:acrion/image_private.git # TODO
        GIT_TAG master
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(acrion_image)
endif ()

include(${acrion_cmake_SOURCE_DIR}/set-cpp-version.cmake)
include(${acrion_cmake_SOURCE_DIR}/get-project-version-from-git-tag.cmake)
get_project_version_from_git_tag("origin/main")

project(acrion_image_tools VERSION ${version_major}.${version_minor}.${version_patch} DESCRIPTION "nexuslua plugin providing basic image processing functions including I/O")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/nexuslua_plugin.toml.template ${CMAKE_CURRENT_BINARY_DIR}/nexuslua_plugin.toml)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version_acrion_image_tools_generated.hpp.cmake ${CMAKE_CURRENT_BINARY_DIR}/version_acrion_image_tools_generated.hpp)

include(${acrion_cmake_SOURCE_DIR}/find-openmp.cmake)
find_package(ZLIB REQUIRED)
include(ExternalProject)
include(GenerateExportHeader)

# Define a single installation directory for all our dependencies
set(DEPS_INSTALL_DIR "${CMAKE_BINARY_DIR}/deps")
set(IMAGEMAGICK_INSTALL_DIR "${DEPS_INSTALL_DIR}")

# Define a success flag to avoid rebuilding
set(ImageMagickSuccessFlag "${IMAGEMAGICK_INSTALL_DIR}/imagemagick_build_success.flag")

option(IMAGEMAGICK_ENABLE_HDRI "Build ImageMagick with HDRI support" OFF)

# Construct the correct library filename suffix based on the option
if (IMAGEMAGICK_ENABLE_HDRI)
    set(IM_FILENAME_SUFFIX "-7.Q32HDRI")
else ()
    set(IM_FILENAME_SUFFIX "-7.Q32")
endif ()

message(STATUS "ImageMagick filename suffix set to: ${IM_FILENAME_SUFFIX}")

# Define file paths based on platform before the external project
if (WIN32)
    set(IM_SUFFIX ".dll.a")
    set(IM_PREFIX "lib")
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(IM_SUFFIX ".dylib")
    set(IM_PREFIX "lib")
else () # Linux
    set(IM_SUFFIX ".so")
    set(IM_PREFIX "lib")
endif ()


if (NOT EXISTS "${ImageMagickSuccessFlag}")
    message(STATUS "ImageMagick build flag not found. Starting custom build process...")

    if (WIN32)
        # --- Windows (MSYS2/UCRT64) Build ---
        set(IM_BUILD_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/im/build_msys2.sh")
        set(IM_SOURCE_DIR "${CMAKE_BINARY_DIR}/imagemagick-build-msys2")
        set(IM_STAGING_DIR "${IM_SOURCE_DIR}/mingw-w64-imagemagick/pkg/mingw-w64-ucrt-x86_64-imagemagick/ucrt64")

        ExternalProject_Add(imagemagick_ext
            GIT_REPOSITORY https://github.com/msys2/MINGW-packages.git
            GIT_TAG master
            SOURCE_DIR "${IM_SOURCE_DIR}"
            CONFIGURE_COMMAND ${CMAKE_COMMAND} -E true
            PATCH_COMMAND     ${CMAKE_COMMAND} -E true
            UPDATE_COMMAND    ${CMAKE_COMMAND} -E true
            BUILD_IN_SOURCE   1

            LOG_DOWNLOAD 0  LOG_UPDATE 0  LOG_CONFIGURE 0  LOG_BUILD 0  LOG_INSTALL 0
            USES_TERMINAL_DOWNLOAD 1
            USES_TERMINAL_UPDATE   1
            USES_TERMINAL_CONFIGURE 1
            USES_TERMINAL_BUILD    1
            USES_TERMINAL_INSTALL  1

            BUILD_COMMAND C:/tools/msys64/msys2_shell.cmd -ucrt64 -defterm -no-start -here -c "bash --noprofile --norc ${IM_BUILD_SCRIPT}"

            # Copy the staged results from makepkg's staging dir to our final destination
            INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory "${IM_STAGING_DIR}" "${IMAGEMAGICK_INSTALL_DIR}"
        )
    elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
        # --- Arch Linux Build ---
        set(IM_BUILD_DIR "${CMAKE_BINARY_DIR}/imagemagick-build-arch")
        set(IM_PATCH_FILE "${CMAKE_CURRENT_SOURCE_DIR}/im/PKGBUILD-arch.patch")

        ExternalProject_Add(imagemagick_ext
            GIT_REPOSITORY https://gitlab.archlinux.org/archlinux/packaging/packages/imagemagick.git
            GIT_TAG main
            SOURCE_DIR "${IM_BUILD_DIR}"
            CONFIGURE_COMMAND ${CMAKE_COMMAND} -E true
            PATCH_COMMAND     ${CMAKE_COMMAND} -E true
            UPDATE_COMMAND    ${CMAKE_COMMAND} -E true
            BUILD_IN_SOURCE 1

            LOG_DOWNLOAD 0  LOG_UPDATE 0  LOG_CONFIGURE 0  LOG_BUILD 0  LOG_INSTALL 0
            USES_TERMINAL_DOWNLOAD 1
            USES_TERMINAL_UPDATE   1
            USES_TERMINAL_CONFIGURE 1
            USES_TERMINAL_BUILD    1
            USES_TERMINAL_INSTALL  1

            BUILD_COMMAND bash -c "patch < '${IM_PATCH_FILE}' &&  makepkg --nodeps --noconfirm --nocheck"

            # Copy the staged results from makepkg's staging dir to our final destination
            INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/pkg/imagemagick/usr ${IMAGEMAGICK_INSTALL_DIR}
        )
    elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
        # --- macOS (Homebrew) Build TODO not yet fully implemented ---
        set(IM_FORMULA_PATH "${CMAKE_CURRENT_SOURCE_DIR}/im/imagemagick.rb")

        ExternalProject_Add(imagemagick_ext
            CONFIGURE_COMMAND ${CMAKE_COMMAND} -E true
            PATCH_COMMAND     ${CMAKE_COMMAND} -E true
            UPDATE_COMMAND    ${CMAKE_COMMAND} -E true
            BUILD_IN_SOURCE 1

            LOG_DOWNLOAD 0  LOG_UPDATE 0  LOG_CONFIGURE 0  LOG_BUILD 0  LOG_INSTALL 0
            USES_TERMINAL_DOWNLOAD 1
            USES_TERMINAL_UPDATE   1
            USES_TERMINAL_CONFIGURE 1
            USES_TERMINAL_BUILD    1
            USES_TERMINAL_INSTALL  1

            BUILD_COMMAND bash -c "brew install pkg-config freetype jpeg-turbo libpng libtiff libtool little-cms2 openjpeg webp xz && brew install --build-from-source --formula '${IM_FORMULA_PATH}'"
            INSTALL_PREFIX ${IMAGEMAGICK_INSTALL_DIR}

            # Copy the staged results from makepkg's staging dir to our final destination
            INSTALL_COMMAND ""
        )
    else ()
        message(FATAL_ERROR "Unsupported platform for custom ImageMagick build.")
    endif ()

    add_custom_command(
        OUTPUT "${ImageMagickSuccessFlag}"
        BYPRODUCTS
        "${IMAGEMAGICK_INSTALL_DIR}/lib/${IM_PREFIX}Magick++${IM_FILENAME_SUFFIX}${IM_SUFFIX}"
        "${IMAGEMAGICK_INSTALL_DIR}/lib/${IM_PREFIX}MagickWand${IM_FILENAME_SUFFIX}${IM_SUFFIX}"
        "${IMAGEMAGICK_INSTALL_DIR}/lib/${IM_PREFIX}MagickCore${IM_FILENAME_SUFFIX}${IM_SUFFIX}"
        COMMAND ${CMAKE_COMMAND} -E touch "${ImageMagickSuccessFlag}"
        DEPENDS imagemagick_ext
        COMMENT "Marking ImageMagick build as complete and declaring outputs."
        VERBATIM
    )
    add_custom_target(imagemagick_build_target ALL DEPENDS "${ImageMagickSuccessFlag}")

else ()
    message(STATUS "Custom ImageMagick build already complete. Skipping.")
    add_custom_target(imagemagick_build_target ALL)
endif ()


# --- Define IMPORTED targets for our custom-built ImageMagick libraries ---
set(ImageMagick_INCLUDE_DIRS "${IMAGEMAGICK_INSTALL_DIR}/include/ImageMagick-7")

# Create helper variables for filenames
set(IM_CORE_BASENAME "MagickCore${IM_FILENAME_SUFFIX}")
set(IM_WAND_BASENAME "MagickWand${IM_FILENAME_SUFFIX}")
set(IM_PLUS_BASENAME "Magick++${IM_FILENAME_SUFFIX}")

# Platform-specific logic for IMPORTED targets
if (WIN32)
    # For Windows, we need to specify both the runtime DLL and the import library
    add_library(ImageMagick::MagickCore SHARED IMPORTED GLOBAL)
    set_property(TARGET ImageMagick::MagickCore PROPERTY IMPORTED_LOCATION "${IMAGEMAGICK_INSTALL_DIR}/bin/lib${IM_CORE_BASENAME}.dll")
    set_property(TARGET ImageMagick::MagickCore PROPERTY IMPORTED_IMPLIB "${IMAGEMAGICK_INSTALL_DIR}/lib/lib${IM_CORE_BASENAME}.dll.a")

    add_library(ImageMagick::MagickWand SHARED IMPORTED GLOBAL)
    set_property(TARGET ImageMagick::MagickWand PROPERTY IMPORTED_LOCATION "${IMAGEMAGICK_INSTALL_DIR}/bin/lib${IM_WAND_BASENAME}.dll")
    set_property(TARGET ImageMagick::MagickWand PROPERTY IMPORTED_IMPLIB "${IMAGEMAGICK_INSTALL_DIR}/lib/lib${IM_WAND_BASENAME}.dll.a")

    add_library(ImageMagick::Magick++ SHARED IMPORTED GLOBAL)
    set_property(TARGET ImageMagick::Magick++ PROPERTY IMPORTED_LOCATION "${IMAGEMAGICK_INSTALL_DIR}/bin/lib${IM_PLUS_BASENAME}.dll")
    set_property(TARGET ImageMagick::Magick++ PROPERTY IMPORTED_IMPLIB "${IMAGEMAGICK_INSTALL_DIR}/lib/lib${IM_PLUS_BASENAME}.dll.a")
else ()
    # For Unix-like systems, IMPORTED_LOCATION is sufficient
    if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
        set(IM_SUFFIX ".dylib")
    else ()
        set(IM_SUFFIX ".so")
    endif ()

    add_library(ImageMagick::MagickCore SHARED IMPORTED GLOBAL)
    set_property(TARGET ImageMagick::MagickCore PROPERTY IMPORTED_LOCATION "${IMAGEMAGICK_INSTALL_DIR}/lib/lib${IM_CORE_BASENAME}${IM_SUFFIX}")

    add_library(ImageMagick::MagickWand SHARED IMPORTED GLOBAL)
    set_property(TARGET ImageMagick::MagickWand PROPERTY IMPORTED_LOCATION "${IMAGEMAGICK_INSTALL_DIR}/lib/lib${IM_WAND_BASENAME}${IM_SUFFIX}")

    add_library(ImageMagick::Magick++ SHARED IMPORTED GLOBAL)
    set_property(TARGET ImageMagick::Magick++ PROPERTY IMPORTED_LOCATION "${IMAGEMAGICK_INSTALL_DIR}/lib/lib${IM_PLUS_BASENAME}${IM_SUFFIX}")
endif ()

# Add dependencies to ensure the external project runs before these targets are used
add_dependencies(ImageMagick::Magick++ ImageMagick::MagickWand ImageMagick::MagickCore imagemagick_build_target)

# Create a helper INTERFACE library to bundle them together
add_library(imagemagick_interface_libs INTERFACE)
target_link_libraries(imagemagick_interface_libs INTERFACE
    ImageMagick::Magick++
    ImageMagick::MagickWand
    ImageMagick::MagickCore
)
target_include_directories(imagemagick_interface_libs INTERFACE
    "${ImageMagick_INCLUDE_DIRS}"
)

add_compile_definitions(STATIC_MAGICK)
file(GLOB_RECURSE fits_sources ${CMAKE_CURRENT_SOURCE_DIR}/cfitsio/*.c)
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set_source_files_properties(${fits_sources} PROPERTIES COMPILE_DEFINITIONS "macintosh")
endif ()

set(lua_interface
    ../main.lua
    ../GetPixel.svg
    ../SetPixel.svg
    nexuslua_plugin.toml.template
    main.cpp
)

SOURCE_GROUP(lua_interface FILES ${lua_interface})
SOURCE_GROUP(cfitsio FILES ${fits_sources})

foreach (source IN LISTS fits_sources)
    if (MSVC)
        set_source_files_properties(${source} PROPERTIES COMPILE_FLAGS /W0)
    else ()
        set_source_files_properties(${source} PROPERTIES COMPILE_FLAGS -w)
    endif ()
endforeach ()

add_library(${PROJECT_NAME} SHARED
    ${fits_sources}
    ${lua_interface}
    fits.cpp
    fits.hpp
    imagemagick.hpp
    io.cpp
    io.hpp
    version_acrion_image_tools.cpp
    version_acrion_image_tools.hpp
)

add_dependencies(${PROJECT_NAME} imagemagick_build_target)

include(${acrion_cmake_SOURCE_DIR}/set-compiler-flags.cmake)

set(PLUGIN_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

if (WIN32)
    set(OS_FAMILY "Windows")
elseif (APPLE)
    set(OS_FAMILY "Darwin")
else ()
    set(OS_FAMILY "Linux")
endif ()

set(COPY_IM_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/copy_imagemagick_runtime.cmake")
file(WRITE "${COPY_IM_SCRIPT}" [=[
# copy_imagemagick_runtime.cmake (generated)
# -D SRC_ROOT, DEST, OS_FAMILY, IM_FILENAME_SUFFIX

if(NOT DEFINED SRC_ROOT OR NOT DEFINED DEST OR NOT DEFINED OS_FAMILY)
  message(FATAL_ERROR "Required vars missing: SRC_ROOT, DEST, OS_FAMILY")
endif()

message(STATUS "[copy_im] SRC_ROOT='${SRC_ROOT}'")
message(STATUS "[copy_im] DEST='${DEST}'")
message(STATUS "[copy_im] OS_FAMILY='${OS_FAMILY}'")
message(STATUS "[copy_im] IM_FILENAME_SUFFIX='${IM_FILENAME_SUFFIX}'")

if(DEST STREQUAL "")
  message(FATAL_ERROR "[copy_im] DEST must not be empty")
endif()

file(MAKE_DIRECTORY "${DEST}")

if(OS_FAMILY STREQUAL "Windows")
  file(GLOB _dlls "${SRC_ROOT}/bin/*.dll")
  foreach(_f IN LISTS _dlls)
    file(COPY "${_f}" DESTINATION "${DEST}")
  endforeach()

  file(GLOB _im_dirs "${SRC_ROOT}/lib/ImageMagick-*")
  if(NOT _im_dirs)
    message(WARNING "No ImageMagick-* directory found under ${SRC_ROOT}/lib – cannot copy modules/config.")
  else()
    list(GET _im_dirs 0 _im_dir)

    # -- config-*
    file(GLOB _cfg_dirs "${_im_dir}/config-*")
    foreach(_cfg IN LISTS _cfg_dirs)
      get_filename_component(_cfg_base "${_cfg}" NAME) # z.B. config-Q32
      file(MAKE_DIRECTORY "${DEST}/${_cfg_base}")
      file(GLOB _cfg_files "${_cfg}/*")
      foreach(_f IN LISTS _cfg_files)
        file(COPY "${_f}" DESTINATION "${DEST}/${_cfg_base}")
      endforeach()
    endforeach()

    # -- modules-* (coders + filters), incl. .la
    file(GLOB _mod_roots "${_im_dir}/modules-*")
    foreach(_mr IN LISTS _mod_roots)
      get_filename_component(_mods_base "${_mr}" NAME) # e.g. modules-Q32 or modules-Q32HDRI
      foreach(_sub IN ITEMS "coders" "filters")
        set(_dest "${DEST}/${_mods_base}/${_sub}")
        file(MAKE_DIRECTORY "${_dest}")

        file(GLOB _mods_dll "${_mr}/${_sub}/*.dll")
        file(GLOB _mods_la  "${_mr}/${_sub}/*.la")
        foreach(_m IN LISTS _mods_dll _mods_la)
          file(COPY "${_m}" DESTINATION "${_dest}")
        endforeach()
      endforeach()
    endforeach()
  endif()

elseif(OS_FAMILY STREQUAL "Linux" OR OS_FAMILY STREQUAL "Darwin")
  set(_names MagickCore MagickWand Magick++)
  if(OS_FAMILY STREQUAL "Darwin")
    set(_ext ".dylib")
  else()
    set(_ext ".so")
  endif()

  set(_total 0)
  foreach(_n IN LISTS _names)
    if(DEFINED IM_FILENAME_SUFFIX AND NOT IM_FILENAME_SUFFIX STREQUAL "")
      file(GLOB _libs "${SRC_ROOT}/lib/lib${_n}${IM_FILENAME_SUFFIX}${_ext}*")
    else()
      file(GLOB _libs "${SRC_ROOT}/lib/lib${_n}${_ext}*")
    endif()
    foreach(_f IN LISTS _libs)
      if(_f MATCHES "\\.la$")
        # skip .la
      else()
        file(COPY "${_f}" DESTINATION "${DEST}")
        math(EXPR _total "${_total}+1")
      endif()
    endforeach()
  endforeach()
  message(STATUS "[copy_im] Unix: copied ${_total} libraries (.la skipped)")
endif()
]=])

set(_im_copy_stamp "${CMAKE_CURRENT_BINARY_DIR}/.im_runtime_copied.stamp")

add_custom_command(
  OUTPUT "${_im_copy_stamp}"
  COMMAND "${CMAKE_COMMAND}"
    -DSRC_ROOT=${IMAGEMAGICK_INSTALL_DIR}
    -DDEST=${PLUGIN_OUTPUT_DIR}
    -DOS_FAMILY=${OS_FAMILY}
    -DIM_FILENAME_SUFFIX=${IM_FILENAME_SUFFIX}
    -P "${COPY_IM_SCRIPT}"
  COMMAND "${CMAKE_COMMAND}" -E touch "${_im_copy_stamp}"
  DEPENDS imagemagick_build_target
  VERBATIM
  USES_TERMINAL
)

add_custom_target(copy_imagemagick_libs DEPENDS "${_im_copy_stamp}")
add_dependencies(${PROJECT_NAME} copy_imagemagick_libs)

foreach (SharedLib ${SharedLibsToCopy})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SharedLib}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copying dependency ${SharedLib}"
    )
endforeach ()

generate_export_header(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/.. # so other projects can #include "acrion_image_tools/..."
    PRIVATE
    "${ZLIB_INCLUDE_DIR}"
    ${CMAKE_CURRENT_SOURCE_DIR}/cfitsio
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)

    if (WIN32) # msys2
        target_link_libraries(${PROJECT_NAME} PRIVATE acrion_image cbeam imagemagick_interface_libs OpenMP::OpenMP_CXX Threads::Threads stdc++fs libversion.a ws2_32 ZLIB::ZLIB)
    elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        target_link_libraries(${PROJECT_NAME} PRIVATE acrion_image cbeam imagemagick_interface_libs OpenMP::OpenMP_CXX Threads::Threads stdc++fs ZLIB::ZLIB)
    elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_link_libraries(${PROJECT_NAME} PRIVATE acrion_image cbeam imagemagick_interface_libs OpenMP::OpenMP_CXX Threads::Threads ZLIB::ZLIB)
    endif ()
else ()
    target_link_libraries(${PROJECT_NAME} PRIVATE acrion_image cbeam imagemagick_interface_libs OpenMP::OpenMP_CXX ZLIB::ZLIB)
endif ()

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${CMAKE_CURRENT_SOURCE_DIR}/.."
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_BINARY_DIR}/nexuslua_plugin.toml"
            "${CMAKE_CURRENT_SOURCE_DIR}/.."
    COMMENT "Copying library and config for ${PROJECT_NAME}..."
)

if (WIN32)
    add_compile_definitions(FF_NO_UNISTD_H) # avoid inclusion of linux specific header in cfitsio/eval_l.c
endif ()

include(${acrion_cmake_SOURCE_DIR}/do-logging.cmake)
message(STATUS "${PROJECT_NAME}: Linking against custom ImageMagick build via IMPORTED targets.")
message(STATUS "${PROJECT_NAME}: ImageMagick include path handled by interface library.")
message(STATUS "${PROJECT_NAME}: ZLIB_INCLUDE_DIR               = '${ZLIB_INCLUDE_DIR}' ")
project(acrion_image_tools_test VERSION ${version_major}.${version_minor}.${version_patch} DESCRIPTION "unit test executable for acrion image tools")
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    GIT_SHALLOW TRUE
)
if (WIN32)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif ()
FetchContent_MakeAvailable(googletest)
enable_testing()
add_executable(
    ${PROJECT_NAME}
    test.cpp
)

# target_include_directories(
# acrion_image_tools_test
# )
target_link_libraries(
    ${PROJECT_NAME}
    acrion_image_tools
    cbeam
    GTest::gtest_main
)
include(GoogleTest)
#gtest_discover_tests(${PROJECT_NAME})

include(${acrion_cmake_SOURCE_DIR}/run-tests.cmake)
