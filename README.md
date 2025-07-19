# HDF5 ZFP Baseline

## FIXME

1. H5Z-ZFP dependencies requires manually specifying path int `CMakeLists.txt`

### Dependencies

1. cmake  
2. mpi
3. gawk
4. gnuplot

### First set the following variables 

```bash
export CC=mpicc
export CMAKE_PREFIX_PATH=/home/ta1/src/hdf5/install
export HDF5_INSTALL_DIR=/home/ta1/src/hdf5/install 
export ZFP_INSTALL_DIR=/home/ta1/src/zfp/install 
export ZFP_FILTER_INSTALL_DIR=/home/ta1/src/H5Z-ZFP/install 
export HDF5_PLUGIN_PATH=/home/ta1/src/H5Z-ZFP/install/plugin
```

### Build ZFP

```bash
git clone https://github.com/LLNL/zfp.git

export ZFP_INSTALL_DIR=/home/ta1/src/zfp/install

cmake .. -DCMAKE_INSTALL_PREFIX=$ZFP_INSTALL_DIR -DZFP_WITH_CMAKE_CONFIG=ON
make -j$(nproc)
make install
```

### Build ZFP Filter

```bash
git clone https://github.com/LLNL/H5Z-ZFP.git

make HDF5_HOME=$HDF5_INSTALL_DIR \
     ZFP_HOME=$ZFP_FILTER_INSTALL_DIR \
     PREFIX=$ZFP_FILTER_INSTALL_DIR install
```

### Build HDF5

```bash
git clone https://github.com/HDFGroup/hdf5
git switch --detach hdf5-1.14.6

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
