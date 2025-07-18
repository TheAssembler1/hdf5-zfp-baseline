# HDF5 ZFP Baseline

### Dependencies

1. cmake  
2. mpi

### Build HDF5

```bash
export HDF5_INSTALL_DIR=/home/ta1/src/hdf5/install

cmake -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_TESTING=ON \
  -DHDF5_BUILD_TOOLS=ON \
  -DHDF5_ENABLE_PARALLEL=ON \
  -DHDF5_ENABLE_SZIP_SUPPORT=OFF \
  -DCMAKE_INSTALL_PREFIX=$HDF5_INSTALL_DIR \
  ..

make -j$(nproc)
make install
```

### Compiling

First make sure `CMAKE_PREFIX_PATH` is set to HDF5 installation cmake directory and `CC` is set to an MPI compiler.

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```
