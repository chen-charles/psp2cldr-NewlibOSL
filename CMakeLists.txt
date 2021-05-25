cmake_minimum_required(VERSION 3.12)

project(NewlibOSL)

set (CMAKE_CXX_STANDARD 20)

add_library(${PROJECT_NAME} SHARED
    impl.cc io.cc pte/pte.cc mlock.cc pte/mutex.cc pte/semaphore.cc
)

if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_definitions(-fvisibility=hidden)
    target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs)
endif()
