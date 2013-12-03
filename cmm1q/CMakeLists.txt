project(cmm1q)

cmake_minimum_required(VERSION 2.8)

if(NOT CMAKELISTS_LOCAL)
    set(CMAKELISTS_LOCAL "CMakeLists.txt.local" CACHE FILEPATH "")
endif()
include(${CMAKELISTS_LOCAL})

find_package( Boost 1.53.0 REQUIRED COMPONENTS system program_options
  filesystem random)
include_directories(${Boost_INCLUDE_DIRS})
  
include_directories(/home/sorge/local/include)

add_definitions(-std=c++11)
add_definitions(-O3)
add_definitions(-Wall)

add_executable(cmm1q main.cpp)
target_link_libraries(cmm1q ${Boost_LIBRARIES})

install(TARGETS cmm1q RUNTIME DESTINATION bin)

