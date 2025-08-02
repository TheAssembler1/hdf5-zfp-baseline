cmake_minimum_required(VERSION 3.10)
project(hdf5-zfp-baseline C)

# Get compiler to shutup
add_compile_options(-Wno-unused-parameter -Wno-unused-variable -Wno-unused-function)

# Require MPI for CC as per your readme
find_package(MPI REQUIRED)
find_package(HDF5 REQUIRED COMPONENTS C)

# For LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Get PDC install directory from environment variable
if(DEFINED ENV{PDC_INSTALL_DIR})
    set(PDC_INSTALL_DIR $ENV{PDC_INSTALL_DIR})
else()
    message(FATAL_ERROR "Environment variable PDC_INSTALL_DIR is not set.")
endif()

# Include and library paths
include_directories("${PDC_INSTALL_DIR}/include")
link_directories("${PDC_INSTALL_DIR}/lib")

# Optionally find the actual library
find_library(PDC_LIBRARY pdc PATHS "${PDC_INSTALL_DIR}/lib")
if(NOT PDC_LIBRARY)
    message(FATAL_ERROR "PDC library not found in ${PDC_INSTALL_DIR}/lib")
endif()

# Check if MERCURY_INSTALL_DIR env var is set
if(DEFINED ENV{MERCURY_INSTALL_DIR})
    set(MERCURY_INSTALL_DIR $ENV{MERCURY_INSTALL_DIR})
else()
    message(FATAL_ERROR "Environment variable MERCURY_INSTALL_DIR is not set.")
endif()

# Add Mercury include and lib directories
include_directories("${MERCURY_INSTALL_DIR}/include")
link_directories("${MERCURY_INSTALL_DIR}/lib")

# JSON-C 
find_package(json-c REQUIRED)

# Find the mercury library
find_library(MERCURY_LIBRARY mercury PATHS "${MERCURY_INSTALL_DIR}/lib")

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
set(ZFP_BASELINE_SOURCES src/main.c 
                         src/common/common.c 
                         src/common/config.c
                         src/common/util.c
                         src/hdf5_impl/hdf5_io_impl.c 
                         src/pdc_impl/pdc_io_impl.c
                         src/exec_io_impl.c)

add_executable(zfp_baseline ${ZFP_BASELINE_SOURCES})
target_compile_options(zfp_baseline PRIVATE -Wall -Wextra)
target_link_libraries(zfp_baseline PRIVATE
  ${HDF5_LIBRARIES}
  ${H5ZZFP_LIBRARY}
  ${ZFP_LIBRARY}
  ${PDC_LIBRARY}
  ${MERCURY_LIBRARY}
  m
  z
  stdc++
  MPI::MPI_C
  json-c::json-c
)
