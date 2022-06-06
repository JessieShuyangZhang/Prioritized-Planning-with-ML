# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
project(pp)
cmake_minimum_required (VERSION 2.6)
add_subdirectory ("training")
add_subdirectory ("testing")