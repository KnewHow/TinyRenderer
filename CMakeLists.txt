cmake_minimum_required(VERSION 3.0.0)
project(CMakeLists.txt VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(CMakeLists main.cpp tgaimage.h tgaimage.cpp geometry.h geometry.cpp model.h model.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
