cmake_minimum_required(VERSION 3.10)
project(CS6913-Web-Search-Engines)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

# Ensure the correct architecture is used
set(CMAKE_OSX_ARCHITECTURES "arm64")

# Add the executable for your main application
add_executable(Main
        src/main.cpp
        src/PageTable.cpp
        src/InvertedList.cpp
        src/Lexicon.cpp
        src/IndexBuilder.cpp
        src/SearchResult.cpp
        src/QueryProcessor.cpp)

# Include directories (if needed)
include_directories(${PROJECT_SOURCE_DIR}/include)

# Find zlib library
find_package(ZLIB REQUIRED)

## Link zlib to the executable
#target_link_libraries(Main ZLIB::ZLIB)

# Find Boost library (specifically boost_system and boost_asio)
find_package(Boost REQUIRED COMPONENTS system)

# Include Boost directories
include_directories(${Boost_INCLUDE_DIRS})

# Link Boost and Zlib to the executable
target_link_libraries(Main ZLIB::ZLIB ${Boost_LIBRARIES})

## Set Python paths manually
#set(Python3_ROOT_DIR "/Users/dl5214/anaconda3/envs/web-crawler")
#set(Python3_INCLUDE_DIR "/Users/dl5214/anaconda3/envs/web-crawler/include/python3.11")
#set(Python3_LIBRARIES "/Users/dl5214/anaconda3/envs/web-crawler/lib/libpython3.11.dylib")
#
## Manually include Python headers
#include_directories(${Python3_INCLUDE_DIR})
#
## Set the path for pybind11 manually
#set(PYBIND11_PATH "/Users/dl5214/anaconda3/envs/web-crawler/lib/python3.11/site-packages/pybind11/include")
#include_directories(${PYBIND11_PATH})
#
## Use pybind11 directly instead of find_package to avoid wrong configuration
#add_library(pybind11::module INTERFACE IMPORTED)
#set_target_properties(pybind11::module PROPERTIES
#        INTERFACE_INCLUDE_DIRECTORIES "${PYBIND11_PATH}")
#
## Add shared library for Python (the C++-Python interface module)
#add_library(query_processor_interface MODULE
#        src/PageTable.cpp
#        src/InvertedList.cpp
#        src/Lexicon.cpp
#        src/IndexBuilder.cpp
#        src/QueryProcessor.cpp
#        src/SearchResult.cpp)
#
## Link Python and Zlib
#target_link_libraries(query_processor_interface PRIVATE ${Python3_LIBRARIES} ZLIB::ZLIB)
#
## Fix unresolved symbols by enabling dynamic lookup of symbols at runtime
#target_link_options(query_processor_interface PRIVATE "-undefined dynamic_lookup")
#
## Add Python framework linkage for macOS
#find_library(PYTHON_FRAMEWORK Python PATHS /Library/Frameworks /System/Library/Frameworks)
#target_link_libraries(query_processor_interface PRIVATE ${PYTHON_FRAMEWORK})
#
## Set output name for the Python module
#set_target_properties(query_processor_interface PROPERTIES PREFIX "")
#
## Set the build type explicitly to Debug
#set(CMAKE_BUILD_TYPE Debug)