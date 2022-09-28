cmake_minimum_required(VERSION 3.15)
project(BriskPKG)

set(CMAKE_CXX_STANDARD 20)

# Options
option(BriskPKG_Dynamic "Build a dynamic library instead of static." OFF)

# Sources
set(BriskPKG_Src BPKG.hpp BPKG.cpp)
if(${BriskPNG_Dynamic})
	add_library(BriskPKG SHARED ${BriskPKG_Src})
else()
	add_library(BriskPKG ${BriskPKG_Src})
endif()

# Basic SDK
project(BriskPKGTool)

# Sources
add_executable(BriskPKGTool SDK/BPKGTool.cpp SDK/CommandHandler.cpp SDK/CommandHandler.hpp)

# Includes
target_link_libraries(BriskPKGTool PRIVATE BriskPKG)