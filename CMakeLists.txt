cmake_minimum_required(VERSION 3.20)

project(RubiksCubeSolver LANGUAGES CXX)

add_compile_options(-O3 -march=native -flto -DNDEBUG)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(cube_solver 
	main.cpp 
	search.cpp
	display.cpp 
	mechanics.cpp  
	genFlipSlice.cpp
	genTwistSlice.cpp 
	genPhaseTwoTable.cpp
	genFlipTwistTable.cpp 
)
