cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0074 NEW)

project(hdf5-zfp-baseline C)

find_package(HDF5 REQUIRED COMPONENTS C)

set(SOURCES
    src/main.c
)

add_executable(zfp_baseline ${SOURCES})

target_include_directories(zfp_baseline PRIVATE ${HDF5_INCLUDE_DIRS})
target_link_libraries(zfp_baseline PRIVATE ${HDF5_LIBRARIES} m z)

target_compile_options(zfp_baseline PRIVATE -Wall -Wextra)
