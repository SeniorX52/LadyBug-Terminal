#=============================================================================
# CMakeLists.txt for LadybugExport
#
# This CMake configuration builds the Ladybug Stream Export Tool.
# 
# Prerequisites:
#   - Teledyne FLIR Ladybug SDK installed
#   - Visual Studio 2019 or later
#   - CMake 3.16 or later
#
# Build Instructions:
#   1. Set LADYBUG_SDK_DIR environment variable to your SDK installation path
#      Example: set LADYBUG_SDK_DIR=C:\Program Files\Point Grey Research\Ladybug
#
#   2. Create build directory and configure:
#      mkdir build
#      cd build
#      cmake -G "Visual Studio 17 2022" -A x64 ..
#
#   3. Build:
#      cmake --build . --config Release
#
#=============================================================================

cmake_minimum_required(VERSION 3.16)
project(LadybugExport VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

#-----------------------------------------------------------------------------
# Ladybug SDK Configuration
#-----------------------------------------------------------------------------

# Try to find Ladybug SDK
# First check environment variable, then common installation paths

if(DEFINED ENV{LADYBUG_SDK_DIR})
    set(LADYBUG_SDK_DIR "$ENV{LADYBUG_SDK_DIR}")
elseif(DEFINED ENV{PGRROOT})
    set(LADYBUG_SDK_DIR "$ENV{PGRROOT}")
else()
    # Common installation paths
    set(LADYBUG_SEARCH_PATHS
        "C:/Program Files/Point Grey Research/Ladybug"
        "C:/Program Files/FLIR Integrated Imaging Solutions/Ladybug"
        "C:/Program Files/Teledyne FLIR/Ladybug"
        "C:/Program Files (x86)/Point Grey Research/Ladybug"
    )
    
    foreach(path ${LADYBUG_SEARCH_PATHS})
        if(EXISTS ${path})
            set(LADYBUG_SDK_DIR ${path})
            break()
        endif()
    endforeach()
endif()

if(NOT LADYBUG_SDK_DIR)
    message(FATAL_ERROR 
        "Ladybug SDK not found!\n"
        "Please set the LADYBUG_SDK_DIR environment variable to your SDK installation path.\n"
        "Example: set LADYBUG_SDK_DIR=C:\\Program Files\\Point Grey Research\\Ladybug"
    )
endif()

message(STATUS "Found Ladybug SDK at: ${LADYBUG_SDK_DIR}")

# Set include and library directories
set(LADYBUG_INCLUDE_DIR "${LADYBUG_SDK_DIR}/include")
set(LADYBUG_LIB_DIR "${LADYBUG_SDK_DIR}/lib64")

# Check for alternative lib directory names
if(NOT EXISTS ${LADYBUG_LIB_DIR})
    if(EXISTS "${LADYBUG_SDK_DIR}/lib/x64")
        set(LADYBUG_LIB_DIR "${LADYBUG_SDK_DIR}/lib/x64")
    elseif(EXISTS "${LADYBUG_SDK_DIR}/lib64/vs2019")
        set(LADYBUG_LIB_DIR "${LADYBUG_SDK_DIR}/lib64/vs2019")
    elseif(EXISTS "${LADYBUG_SDK_DIR}/lib")
        set(LADYBUG_LIB_DIR "${LADYBUG_SDK_DIR}/lib")
    endif()
endif()

message(STATUS "Ladybug include directory: ${LADYBUG_INCLUDE_DIR}")
message(STATUS "Ladybug library directory: ${LADYBUG_LIB_DIR}")

# Find Ladybug libraries
find_library(LADYBUG_LIBRARY 
    NAMES ladybug LadyBug
    PATHS ${LADYBUG_LIB_DIR}
    NO_DEFAULT_PATH
)

find_library(LADYBUG_GUI_LIBRARY 
    NAMES ladybuggui LadybugGUI
    PATHS ${LADYBUG_LIB_DIR}
    NO_DEFAULT_PATH
)

if(NOT LADYBUG_LIBRARY)
    message(WARNING "Ladybug library not found in ${LADYBUG_LIB_DIR}")
    message(STATUS "Using default library name: ladybug.lib")
    set(LADYBUG_LIBRARY "${LADYBUG_LIB_DIR}/ladybug.lib")
endif()

message(STATUS "Ladybug library: ${LADYBUG_LIBRARY}")

#-----------------------------------------------------------------------------
# Optional: OpenCV Configuration
#-----------------------------------------------------------------------------

option(USE_OPENCV "Use OpenCV for image saving (optional)" OFF)

if(USE_OPENCV)
    find_package(OpenCV QUIET)
    if(OpenCV_FOUND)
        message(STATUS "Found OpenCV: ${OpenCV_VERSION}")
        add_definitions(-DUSE_OPENCV)
    else()
        message(WARNING "OpenCV not found. Image saving will use built-in methods.")
        set(USE_OPENCV OFF)
    endif()
endif()

#-----------------------------------------------------------------------------
# Build Configuration
#-----------------------------------------------------------------------------

# Source files
set(SOURCES
    main.cpp
)

# Create executable
add_executable(LadybugExport ${SOURCES})

# Include directories
target_include_directories(LadybugExport PRIVATE
    ${LADYBUG_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(LadybugExport PRIVATE
    ${LADYBUG_LIBRARY}
)

# Add OpenCV if available
if(USE_OPENCV AND OpenCV_FOUND)
    target_include_directories(LadybugExport PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(LadybugExport PRIVATE ${OpenCV_LIBS})
endif()

# Windows-specific settings
if(WIN32)
    target_compile_definitions(LadybugExport PRIVATE
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
    
    # Link Windows libraries
    target_link_libraries(LadybugExport PRIVATE
        Shlwapi
    )
endif()

# Compiler flags
if(MSVC)
    target_compile_options(LadybugExport PRIVATE
        /W4
        /WX-
        /MP
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Debug>:/Od /Zi>
    )
else()
    target_compile_options(LadybugExport PRIVATE
        -Wall
        -Wextra
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Debug>:-g>
    )
endif()

#-----------------------------------------------------------------------------
# Installation
#-----------------------------------------------------------------------------

install(TARGETS LadybugExport
    RUNTIME DESTINATION bin
)

# Copy required DLLs for Windows
if(WIN32)
    set(LADYBUG_BIN_DIR "${LADYBUG_SDK_DIR}/bin64")
    if(NOT EXISTS ${LADYBUG_BIN_DIR})
        if(EXISTS "${LADYBUG_SDK_DIR}/bin/x64")
            set(LADYBUG_BIN_DIR "${LADYBUG_SDK_DIR}/bin/x64")
        elseif(EXISTS "${LADYBUG_SDK_DIR}/bin")
            set(LADYBUG_BIN_DIR "${LADYBUG_SDK_DIR}/bin")
        endif()
    endif()
    
    # Find and copy DLLs
    file(GLOB LADYBUG_DLLS "${LADYBUG_BIN_DIR}/*.dll")
    
    add_custom_command(TARGET LadybugExport POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${LADYBUG_DLLS}
            $<TARGET_FILE_DIR:LadybugExport>
        COMMENT "Copying Ladybug SDK DLLs..."
    )
endif()

#-----------------------------------------------------------------------------
# Print Summary
#-----------------------------------------------------------------------------

message(STATUS "")
message(STATUS "=== Build Configuration Summary ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Ladybug SDK: ${LADYBUG_SDK_DIR}")
message(STATUS "Use OpenCV: ${USE_OPENCV}")
message(STATUS "===================================")
message(STATUS "")
