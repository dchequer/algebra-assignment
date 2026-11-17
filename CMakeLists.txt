cmake_minimum_required(VERSION 3.24)
project(algebra_assignment VERSION 0.1.0 LANGUAGES CXX)

set(ALGEBRA_ASSIGNMENT_SIZE 4000 CACHE STRING "Square matrix dimension")
if(NOT ALGEBRA_ASSIGNMENT_SIZE MATCHES "^[1-9][0-9]*$")
    message(FATAL_ERROR "The matrix dimension must be a positive integer")
endif()

set(ALGEBRA_ASSIGNMENT_BACKEND "original" CACHE STRING "Finite-field backend: original or cached")
set_property(CACHE ALGEBRA_ASSIGNMENT_BACKEND PROPERTY STRINGS original cached)
if(NOT ALGEBRA_ASSIGNMENT_BACKEND MATCHES "^(original|cached)$")
    message(FATAL_ERROR "ALGEBRA_ASSIGNMENT_BACKEND must be original or cached")
endif()
message(STATUS "Finite-field backend: ${ALGEBRA_ASSIGNMENT_BACKEND}")

include(cmake/Dependencies.cmake)
add_subdirectory(src)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
