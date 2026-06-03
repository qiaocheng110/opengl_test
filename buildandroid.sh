#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
NDK_ROOT="/home/work2/NDK/android-ndk-r26b"
BUILD_DIR="${ROOT_DIR}/build-android"
INSTALL_DIR="${ROOT_DIR}/android-out"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_TOOLCHAIN_FILE="${NDK_ROOT}/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26 \
  -DANDROID_STL=c++_shared \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}"

cmake --build "${BUILD_DIR}" --config Release -j"$(nproc)"

mkdir -p "${INSTALL_DIR}/bin" "${INSTALL_DIR}/lib64/opencv"
cp "${BUILD_DIR}/opencv_benchmark" "${BUILD_DIR}/gles_benchmark" "${INSTALL_DIR}/bin/"
cp "${ROOT_DIR}/lib64/libEGL.so" "${ROOT_DIR}/lib64/libGLESv3.so" "${INSTALL_DIR}/lib64/"
cp "${ROOT_DIR}/lib64/opencv/"*.so "${INSTALL_DIR}/lib64/opencv/"
cp "${NDK_ROOT}/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so" "${INSTALL_DIR}/lib64/"

echo "Built:"
echo "  ${INSTALL_DIR}/bin/opencv_benchmark"
echo "  ${INSTALL_DIR}/bin/gles_benchmark"
