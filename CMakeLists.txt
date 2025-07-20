cmake_minimum_required(VERSION 3.10)
project(hdf5-zfp-baseline C)

# Require MPI for CC as per your readme
find_package(MPI REQUIRED)
find_package(HDF5 REQUIRED COMPONENTS C)

# For LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find ZFP library and include directories (allow user to specify)
find_path(ZFP_INCLUDE_DIR zfp.h
  HINTS ENV ZFP_INSTALL_DIR
  PATH_SUFFIXES include
)

find_library(ZFP_LIBRARY
  NAMES zfp
  HINTS ENV ZFP_INSTALL_DIR
  PATH_SUFFIXES lib
)

# Find H5Z-ZFP plugin static library and includes (user provided)
find_path(H5ZZFP_INCLUDE_DIR H5Zzfp.h
  HINTS ENV ZFP_FILTER_INSTALL_DIR
  PATH_SUFFIXES include
)

find_library(H5ZZFP_LIBRARY
  NAMES h5zzfp
  HINTS ENV ZFP_FILTER_INSTALL_DIR
  PATH_SUFFIXES lib
)

if(NOT ZFP_INCLUDE_DIR OR NOT ZFP_LIBRARY)
  message(FATAL_ERROR "ZFP library not found. Set ZFP_INSTALL_DIR environment variable correctly.")
endif()

if(NOT H5ZZFP_INCLUDE_DIR OR NOT H5ZZFP_LIBRARY)
  message(FATAL_ERROR "H5Z-ZFP plugin not found. Set ZFP_FILTER_INSTALL_DIR environment variable correctly.")
endif()

# Set include dirs and link directories
include_directories(${HDF5_INCLUDE_DIRS} ${ZFP_INCLUDE_DIR} ${H5ZZFP_INCLUDE_DIR})
link_directories(${ZFP_LIBRARY} ${H5ZZFP_LIBRARY})

# List your executables and sources
set(SOURCES1 src/main.c)
set(SOURCES2 src/simple_main.c)

add_executable(zfp_baseline ${SOURCES1})
add_executable(simple_zfp ${SOURCES2})

# Common compile options
target_compile_options(zfp_baseline PRIVATE -Wall -Wextra)
target_compile_options(simple_zfp PRIVATE -Wall -Wextra)

# Link libraries
foreach(target zfp_baseline simple_zfp)
  target_link_libraries(${target} PRIVATE
    ${HDF5_LIBRARIES}
    ${H5ZZFP_LIBRARY}
    ${ZFP_LIBRARY}
    m
    z
    stdc++
    MPI::MPI_C
  )
endforeach()

