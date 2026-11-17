add_executable(algebra_determinants main.cpp)

# add include directories for "determinant.hpp"
target_include_directories(algebra_determinants PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_features(algebra_determinants PRIVATE cxx_std_20)
target_compile_definitions(algebra_determinants PRIVATE
    ALGEBRA_ASSIGNMENT_SIZE=${ALGEBRA_ASSIGNMENT_SIZE}
)

# # toggle profiling flags for this target
# target_compile_options(algebra_determinants PRIVATE -pg -g)
# target_link_options(algebra_determinants PRIVATE -pg)

target_link_libraries(algebra_determinants PRIVATE
    algebra::finite_fields
    algebra::matrices
)
