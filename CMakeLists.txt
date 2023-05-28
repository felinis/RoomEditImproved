cmake_minimum_required(VERSION 3.14)

project("Sabre Engine")

# filesystem library
add_subdirectory("sbfilesystem")

# memory library
add_subdirectory("sbmemory")

# input library
add_subdirectory("sbinput")

# audio library
add_subdirectory("sbaudio")

# graphics library
add_subdirectory("sbgraphics")

# room editor executable
add_subdirectory("roomedit")
