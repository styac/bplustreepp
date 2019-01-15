cmake_minimum_required(VERSION 3.10)

project(bplustreecpp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -Wall" )
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS}" )
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS}" )

file( GLOB HEADERS ${PROJECT_SOURCE_DIR}
    src/*.h
)

# no effect
# source_group("HEADERS" FILES ${HEADERS})

set( SOURCES ${PROJECT_SOURCE_DIR}
    src/btree.cpp
    src/sha1tiny.cpp
    test/test_btree.cpp
)

# no effect
# source_group("SOURCES" FILES ${SOURCES})

include_directories(
    src
)

add_executable( ${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

#target_link_libraries( ${PROJECT_NAME}
#    sndfile
#)

