project(hikvisionbroker_extension C CXX)
cmake_minimum_required(VERSION 3.13)
set (CMAKE_CXX_STANDARD 14)
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
include_directories(${Python3_INCLUDE_DIRS})

find_package(pybind11 CONFIG REQUIRED)
include_directories(${pybind11_INCLUDE_DIRS})

find_package(rxcpp CONFIG)

find_package(PkgConfig REQUIRED)
pkg_search_module(TURBOJPEG REQUIRED libturbojpeg)
link_directories(${TURBOJPEG_LIBDIR})
include_directories(${TURBOJPEG_INCLUDE_DIRS})

# MVS SDK
message("MVS Library Path: $ENV{LD_LIBRARY_PATH}")
set(MVS_RUNTIME_LIB $ENV{LD_LIBRARY_PATH})
link_directories(${MVS_RUNTIME_LIB})
include_directories(/opt/MVS/include)


pybind11_add_module(hikvisionbroker_extension module.cpp camera.cpp)
target_compile_definitions(${PROJECT_NAME} PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})

#set_target_properties(${PROJECT_NAME}  PROPERTIES LINK_FLAGS "-Wl,-rpath=/opt/MVS/lib/64 -Wl,--disable-new-dtags")
target_link_libraries(${PROJECT_NAME} PRIVATE pthread MvCameraControl ${TURBOJPEG_LIBRARIES} jpeg)


