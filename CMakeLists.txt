cmake_minimum_required(VERSION 3.0)
project(qed_sampler)

set(CMAKE_CXX_STANDARD 11)
find_package(PkgConfig)

if (${jsoncpp_FOUND})
    MESSAGE(STATUS "jsoncpp found via CMake")
else (${jsoncpp_FOUND})
    MESSAGE(STATUS "jsoncpp not found via CMake, trying pkg-config")
    pkg_search_module(JSONCPP REQUIRED jsoncpp)
endif (${jsoncpp_FOUND})


add_executable(qed_sampler main.cpp qta.h io.h CLI11.hpp)
target_link_libraries(qed_sampler ${JSONCPP_LIBRARIES} jsoncpp)
target_include_directories(qed_sampler PUBLIC ${JSONCPP_INCLUDE_DIRS})

install(TARGETS qed_sampler)