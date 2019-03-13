#!/bin/bash
export PATH=/opt/lofar/cmake/bin/:$PATH

export INSTALLDIR=/opt/lofar

export CMAKE_PREFIX_PATH=$INSTALLDIR/aoflagger:$INSTALLDIR/armadillo:$INSTALLDIR/boost:$INSTALLDIR/casacore:$INSTALLDIR/cfitsio:$INSTALLDIR/hdf5:$INSTALLDIR/superlu:$INSTALLDIR/lofar:$INSTALLDIR/LOFARBeam:$INSTALLDIR/hdf5
export LD_LIBRARY_PATH=$INSTALLDIR/hdf5/lib:$INSTALLDIR/superlu/lib64:$INSTALLDIR/LOFARBeam/lib:$LD_LIBRARY_PATH
export HDF5_ROOT=/opt/lofar/hdf5/
export PAPI_INCLUDE_DIR=/home/apmechev/papi_tests/

export CPLUS_INCLUDE_PATH=/home/apmechev/GRIDTOOLS/papi/src

cmake ..
make clean
make  -j4


