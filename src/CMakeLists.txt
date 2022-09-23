cmake_minimum_required(VERSION 3.10) 

# set the project name 
project(uniqwordcount VERSION 1.0)

file(GLOB SOURCES
    *.h
    *.cpp
)
# add the executable
add_executable(uniqwordcount ${SOURCES})
set_target_properties(uniqwordcount PROPERTIES
									CXX_STANDARD 17
									CXX_STANDARD_REQUIRED OFF
									CXX_EXTENSIONS OFF)
find_package( Threads )
target_link_libraries( uniqwordcount ${CMAKE_THREAD_LIBS_INIT} )
