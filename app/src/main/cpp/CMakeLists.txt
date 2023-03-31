# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.22.1)

# Declares and names the project.

project("tcamera")

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

add_library( # Sets the name of the library.
        tcamera

        # Sets the library as a shared library.
        SHARED
        ${CMAKE_CURRENT_SOURCE_DIR}/ncamera.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/camera_listeners.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/camera_surface.cpp
        native-lib.cpp)


target_link_libraries( # Specifies the target library.
        tcamera
        log
        android
        camera2ndk)