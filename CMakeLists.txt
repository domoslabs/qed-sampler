cmake_minimum_required(VERSION 3.16)
project(qed_sampler)

set(CMAKE_CXX_STANDARD 11)

add_executable(qed_sampler main.cpp qta.cpp qta.h io.cpp io.h CLI11.hpp)
