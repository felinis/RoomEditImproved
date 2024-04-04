cmake_minimum_required(VERSION 3.21)

project("Room Editor" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# filesystem library
add_subdirectory("sbfilesystem")

# memory library
add_subdirectory("sbmemory")

# graphics library
add_subdirectory("sbgraphics")

# room editor executable
add_subdirectory("roomedit")
