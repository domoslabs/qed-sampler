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


find_library(LIBTDIGEST libtdigest.so)

add_executable(qed-sampler main.cpp qta.h io.h CLI11.hpp)
target_link_libraries(qed-sampler ${JSONCPP_LIBRARIES} jsoncpp)
target_link_libraries(qed-sampler ${LIBTDIGEST})
target_include_directories(qed-sampler PUBLIC ${JSONCPP_INCLUDE_DIRS})

install(TARGETS qed-sampler)