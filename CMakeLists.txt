cmake_minimum_required(VERSION 3.18)
project(nv12_rgb_benchmarks LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(PREBUILT_LIB_DIR "${PROJECT_ROOT}/lib64")
set(OPENCV_LIB_DIR "${PREBUILT_LIB_DIR}/opencv")

include_directories("${PROJECT_ROOT}/include")

add_library(prebuilt_egl SHARED IMPORTED)
set_target_properties(prebuilt_egl PROPERTIES IMPORTED_LOCATION "${PREBUILT_LIB_DIR}/libEGL.so")

add_library(prebuilt_glesv3 SHARED IMPORTED)
set_target_properties(prebuilt_glesv3 PROPERTIES IMPORTED_LOCATION "${PREBUILT_LIB_DIR}/libGLESv3.so")

add_library(opencv_core SHARED IMPORTED)
set_target_properties(opencv_core PROPERTIES IMPORTED_LOCATION "${OPENCV_LIB_DIR}/libopencv_core.so")

add_library(opencv_imgproc SHARED IMPORTED)
set_target_properties(opencv_imgproc PROPERTIES IMPORTED_LOCATION "${OPENCV_LIB_DIR}/libopencv_imgproc.so")

add_executable(opencv_benchmark src/opencv_benchmark.cpp)
target_link_libraries(opencv_benchmark PRIVATE opencv_imgproc opencv_core)

add_executable(gles_benchmark src/gles_benchmark.cpp)
target_link_libraries(gles_benchmark PRIVATE prebuilt_egl prebuilt_glesv3)

foreach(target opencv_benchmark gles_benchmark)
    target_include_directories(${target} PRIVATE "${PROJECT_ROOT}/src")
    if(ANDROID)
        # Route logging through Android logcat (<android/log.h> -> liblog).
        find_library(ANDROID_LOG_LIB log)
        target_link_libraries(${target} PRIVATE ${ANDROID_LOG_LIB})
    endif()
endforeach()
