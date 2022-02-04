cmake_minimum_required(VERSION 3.12)

project(NewlibOSL)

set (CMAKE_CXX_STANDARD 20)

add_library(${PROJECT_NAME} SHARED
    impl.cc io.cc mlock.cc pte/pte.cc pte/mutex.cc pte/semaphore.cc pte/thread.hpp target.h
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PSP2CLDR REQUIRED psp2cldr)
target_link_libraries(${PROJECT_NAME} ${PSP2CLDR_LIBRARIES})
target_include_directories(${PROJECT_NAME} PUBLIC ${PSP2CLDR_INCLUDE_DIRS})
target_compile_options(${PROJECT_NAME} PUBLIC ${PSP2CLDR_CFLAGS_OTHER})

if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_definitions(-fvisibility=hidden)
    target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs)
endif()
