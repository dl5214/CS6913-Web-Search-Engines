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

# Link zlib to the executable
target_link_libraries(Main ZLIB::ZLIB)

# Find Boost library (specifically boost_system and boost_asio)
find_package(Boost REQUIRED COMPONENTS system)

# Include Boost directories
include_directories(${Boost_INCLUDE_DIRS})

# Link Boost and Zlib to the executable
target_link_libraries(Main ZLIB::ZLIB ${Boost_LIBRARIES})
