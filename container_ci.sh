#!/bin/sh -eu

cd "$(dirname "${0}")"

for COMPILER in "gcc" "clang"; do
  for BUILD_TYPE in "Debug" "Release"; do
    rm -rf "build"
    mkdir "build"
    cd "build"
      if test "${COMPILER}" = "gcc"; then
        CMAKE_COMPILER_OPTIONS="-DCMAKE_C_COMPILER=$(which "gcc")"
      else
        CMAKE_COMPILER_OPTIONS="-DCMAKE_C_COMPILER=$(which "clang")"
      fi
      cmake .. \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DPOPORON_USE_TESTS=ON \
        -DPOPORON_USE_VALGRIND=ON \
        ${CMAKE_COMPILER_OPTIONS}
      cmake --build . --parallel
      ctest -C "${BUILD_TYPE}" --output-on-failure
    cd -
  done
done
