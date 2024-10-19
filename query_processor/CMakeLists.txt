cmake_minimum_required(VERSION 3.10)
project(SearchSystem)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

## Add the first executable for creating the index
#add_executable(CreateIndex src/create_index.cpp)
#
## Add the second executable for merging and compressing the index
#add_executable(MergeIndex src/merge_index.cpp)

add_executable(Main
        src/main.cpp
        src/PageTable.cpp
        src/InvertedList.cpp
        src/Lexicon.cpp
        src/IndexBuilder.cpp
        )

# Include directories (if needed)
include_directories(${PROJECT_SOURCE_DIR}/include)

# Find zlib library
find_package(ZLIB REQUIRED)

# Link zlib to the executable
target_link_libraries(Main ZLIB::ZLIB)

## Link libraries (for filesystem support on macOS)
#if (APPLE)
#    target_link_libraries(CreateIndex stdc++fs)
#    target_link_libraries(MergeIndex stdc++fs)
#endif()