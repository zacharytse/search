cmake_minimum_required(VERSION 3.10.2)
project(search_project)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS -pthread)
include_directories(/usr/include/boost)
add_executable(search_project main.cpp offline/index_builder.cpp offline/index_builder.h 3rd/json/CJsonObject.h 3rd/json/CJsonObject.cpp 3rd/json/cJSON.h 3rd/json/cJSON.c utils/spin_lock.cpp utils/spin_lock.h index/invert_index.h online/search_logic.cpp online/search_logic.h service/search_service.cpp service/search_service.h index/forward_index.h service/search.h utils/segment.h index/indexer.cpp index/indexer.h utils/singleton.h)
