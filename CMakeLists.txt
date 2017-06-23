cmake_minimum_required(VERSION 3.7)
project(ServerEx4)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")


set(SOURCE_FILES )
add_executable(ServerEx4 main.c)
add_executable(ClientEx4 ClientEx4.c)
target_link_libraries(ServerEx4  pthread)
target_link_libraries(ClientEx4 pthread)