cmake_minimum_required(VERSION 3.12)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(tests)

# ideally this should be set by the toolchain, but since we are using vitasdk's files, just force it here
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_ENABLE_EXPORTS ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include("${VITASDK}/share/vita.cmake" REQUIRED)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
  
FILE(GLOB SOURCES *.c)
foreach(SRC_FILENAME ${SOURCES})
    get_filename_component(SRC_NAME ${SRC_FILENAME} NAME_WLE)

    add_library(${SRC_NAME} SHARED
        ${SRC_NAME}.c
    )
    target_link_libraries(${SRC_NAME} PRIVATE Threads::Threads)

    set_target_properties(${SRC_NAME} PROPERTIES PREFIX "")
endforeach()
