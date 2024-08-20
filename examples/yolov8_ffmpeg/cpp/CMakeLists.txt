cmake_minimum_required(VERSION 3.10)

project(rknn_yolov8_ffmpeg_demo)

# AddressSanitizer option
if (ENABLE_ASAN)
    message(STATUS "BUILD WITH ADDRESS SANITIZER")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif()

# RKNN-specific files and configurations
set(rknpu_yolov8_file rknpu2/yolov8.cc)

if (TARGET_SOC STREQUAL "rv1106" OR TARGET_SOC STREQUAL "rv1103")
    add_definitions(-DRV1106_1103)
    set(rknpu_yolov8_file rknpu2/yolov8_rv1106_1103.cc)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/allocator/dma)
endif()

if (TARGET_SOC STREQUAL "rk1808" OR TARGET_SOC STREQUAL "rv1109" OR TARGET_SOC STREQUAL "rv1126")
    add_definitions(-DRKNPU1)
    set(rknpu_yolov8_file rknpu1/yolov8.cc)
endif()

# Adding third-party subdirectories
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/ 3rdparty.out)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/ utils.out)

set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

# Source files
file(GLOB SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

# Add the executable
add_executable(${PROJECT_NAME}
    main.cc
    postprocess.cc
    ${rknpu_yolov8_file}
)

# SDL2 and SDL_ttf setup
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/../)

target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})

# FFmpeg (libav*) setup
include_directories(/usr/include/x86_64-linux-gnu/libavcodec)
include_directories(/usr/include/x86_64-linux-gnu/libavformat)
include_directories(/usr/include/x86_64-linux-gnu/libavutil)

set(AVCODEC_LIB "/usr/lib/x86_64-linux-gnu/libavcodec")
set(AVFORMAT_LIB "/usr/lib/x86_64-linux-gnu/libavformat")
set(AVUTIL_LIB "/usr/lib/x86_64-linux-gnu/libavutil")


target_link_libraries(${PROJECT_NAME}
    imageutils
    fileutils
    imagedrawing
    libavcodec
    libavformat
    libavutil
    ${LIBRKNNRT}
    dl
)

# Android-specific linking
if (CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_libraries(${PROJECT_NAME} log)
endif()

# Linux-specific linking for pthread
message(STATUS "!!!!!!!!!!!CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} Threads::Threads)
endif()

# Installation instructions
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBRKNNRT_INCLUDES}
)

install(TARGETS ${PROJECT_NAME} DESTINATION .)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/bus.jpg DESTINATION model)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../model/coco_80_labels_list.txt DESTINATION model)
file(GLOB RKNN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../model/*.rknn")
install(FILES ${RKNN_FILES} DESTINATION model)
