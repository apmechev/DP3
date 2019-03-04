rm -rf build
mkdir -p build
cd build

singularity exec shub://tikk3r/lofar-grid-hpccloud:lofar  ../build_cmake.sh
