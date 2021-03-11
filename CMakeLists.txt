cmake_minimum_required(VERSION 3.0.0)
project(CMakeLists.txt VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(CMakeLists main.cpp tgaimage.h tgaimage.cpp geometry.h geometry.cpp model.h model.cpp ourGL.h ourGL.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()
