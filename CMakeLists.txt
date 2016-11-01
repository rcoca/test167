project(test167)
cmake_minimum_required(VERSION 2.8)
add_definitions("-std=c++14")
add_definitions("-ftemplate-backtrace-limit=0")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall  -pedantic")
find_package( Boost REQUIRED COMPONENTS  system thread)
find_package(Threads)

set(SRC_LIST main.cpp)

add_executable(${PROJECT_NAME} ${SRC_LIST})


target_link_libraries(${PROJECT_NAME}
    ${Boost_LIBRARIES}
    ${PTHREAD_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
)
