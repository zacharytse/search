cmake_minimum_required(VERSION 3.10.2)
project(search_project)

set(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_FLAGS -pthread)
add_executable(search_project main.cpp offline/index_builder.cpp offline/index_builder.h 3rd/json/CJsonObject.h 3rd/json/CJsonObject.cpp 3rd/json/cJSON.h 3rd/json/cJSON.c utils/spin_lock.cpp utils/spin_lock.h)
