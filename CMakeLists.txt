cmake_minimum_required(VERSION 3.0)
project(qed-sampler)

set(CMAKE_CXX_STANDARD 11)
find_package(PkgConfig)


if (${jsoncpp_FOUND})
    MESSAGE(STATUS "jsoncpp found via CMake")
else (${jsoncpp_FOUND})
    MESSAGE(STATUS "jsoncpp not found via CMake, trying pkg-config")
    pkg_search_module(JSONCPP REQUIRED jsoncpp)
endif (${jsoncpp_FOUND})


find_library(LIBTDIGESTCPP libtdigestcpp.so)

add_executable(qed-sampler src/main.cpp include/qta.h include/io.h include/CLI11.hpp)
target_link_libraries(qed-sampler ${JSONCPP_LIBRARIES} jsoncpp)
target_link_libraries(qed-sampler ${LIBTDIGESTCPP})
target_include_directories(qed-sampler PUBLIC ${JSONCPP_INCLUDE_DIRS})

install(TARGETS qed-sampler)