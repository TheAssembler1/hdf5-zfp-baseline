cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0074 NEW)

project(hdf5-zfp-baseline C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(HDF5 REQUIRED COMPONENTS C)

set(SOURCES
    src/main.c
)

add_executable(zfp_baseline ${SOURCES})

# Add H5Z-ZFP include directory
target_include_directories(zfp_baseline PRIVATE
    ${HDF5_INCLUDE_DIRS}
    /home/ta1/src/H5Z-ZFP/install/include
)

# Link with HDF5 and zfp library
target_link_libraries(zfp_baseline PRIVATE
    ${HDF5_LIBRARIES}
    /home/ta1/src/H5Z-ZFP/install/lib/libh5zzfp.a
    m
    z
)

target_compile_options(zfp_baseline PRIVATE -Wall -Wextra)
