project(hikvisionbroker_extension C CXX)
cmake_minimum_required(VERSION 3.17)

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
include_directories(${Python3_INCLUDE_DIRS})

find_package(pybind11 CONFIG REQUIRED)
include_directories(${pybind11_INCLUDE_DIRS})

find_package(rxcpp CONFIG)

# MVS SDK
set(MVS_RUNTIME_LIB /opt/MVS/lib/64)
link_directories(${MVS_RUNTIME_LIB})
include_directories(/opt/MVS/include)


pybind11_add_module(hikvisionbroker_extension module.cpp camera.cpp)
target_compile_definitions(${PROJECT_NAME} PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})

#set_target_properties(${PROJECT_NAME}  PROPERTIES LINK_FLAGS "-Wl,-rpath=/opt/MVS/lib/64 -Wl,--disable-new-dtags")
target_link_libraries(${PROJECT_NAME} PRIVATE pthread MvCameraControl)

add_library(mainlib SHARED mainlib.cpp)
#set_target_properties(mainlib PROPERTIES LINK_FLAGS "-Wl,-rpath=/opt/MVS/lib/64 -Wl,--disable-new-dtags")
target_link_libraries(mainlib PRIVATE pthread MvCameraControl)

add_executable(main main.cpp)
target_link_libraries(main mainlib)
