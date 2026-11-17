cmake_minimum_required(VERSION 3.24)
project(algebra_finite_fields_cached VERSION 0.2.0 LANGUAGES CXX)

include(cmake/Dependencies.cmake)

add_library(algebra_finite_fields_cached INTERFACE)
add_library(algebra::finite_fields ALIAS algebra_finite_fields_cached)
target_compile_features(algebra_finite_fields_cached INTERFACE cxx_std_20)
target_include_directories(algebra_finite_fields_cached INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(algebra_finite_fields_cached INTERFACE
    algebra::rings
    algebra::polynomials
)
