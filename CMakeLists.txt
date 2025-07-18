cmake_minimum_required(VERSION 3.10)
project(hdf5-zfp-baseline C)

# Find HDF5 package (C API)
find_package(HDF5 REQUIRED COMPONENTS C)

# List of c source files
set(SOURCES
    src/main.c
)

# Add your executable
add_executable(zfp_baseline main.c)   # Replace main.c with your source files

# Include HDF5 headers
target_include_directories(my_app PRIVATE ${HDF5_INCLUDE_DIRS})

# Link HDF5 libraries
target_link_libraries(my_app PRIVATE ${HDF5_LIBRARIES})

# Optional: enable compiler warnings
target_compile_options(my_app PRIVATE -Wall -Wextra)

