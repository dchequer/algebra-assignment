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

option(ALGEBRA_ASSIGNMENT_GPROF "Instrument the executable for GNU gprof" OFF)
if(ALGEBRA_ASSIGNMENT_GPROF)
    target_compile_options(algebra_determinants PRIVATE -pg -g)
    target_link_options(algebra_determinants PRIVATE -pg)
endif()

target_link_libraries(algebra_determinants PRIVATE
    algebra::finite_fields
    algebra::matrices
)

add_executable(sequential_determinant sequential_determinant.cpp)
target_compile_features(sequential_determinant PRIVATE cxx_std_20)
target_compile_definitions(sequential_determinant PRIVATE
    ALGEBRA_ASSIGNMENT_SIZE=${ALGEBRA_ASSIGNMENT_SIZE}
)
if(ALGEBRA_ASSIGNMENT_BACKEND STREQUAL "cached")
    target_compile_definitions(sequential_determinant PRIVATE ALGEBRA_ASSIGNMENT_BENCHMARK_CACHED)
endif()
target_include_directories(sequential_determinant PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
target_link_libraries(sequential_determinant PRIVATE
    algebra::finite_fields
    algebra::matrices
)

option(ALGEBRA_ASSIGNMENT_BUILD_MPI "Build MPI programs" OFF)

if(ALGEBRA_ASSIGNMENT_BUILD_MPI)
    find_package(MPI REQUIRED COMPONENTS CXX)

    add_executable(mpi_hello mpi_hello.cpp)
    target_compile_features(mpi_hello PRIVATE cxx_std_20)
    target_link_libraries(mpi_hello PRIVATE MPI::MPI_CXX)

    add_executable(mpi_distribute mpi_distribute.cpp)
    target_compile_features(mpi_distribute PRIVATE cxx_std_20)
    target_include_directories(mpi_distribute PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
    )
    target_compile_definitions(mpi_distribute PRIVATE
        ALGEBRA_ASSIGNMENT_SIZE=${ALGEBRA_ASSIGNMENT_SIZE}
    )
    target_link_libraries(mpi_distribute PRIVATE
        MPI::MPI_CXX
        algebra::finite_fields
        algebra::matrices
    )

    add_executable(mpi_determinant mpi_determinant.cpp)
    target_compile_features(mpi_determinant PRIVATE cxx_std_20)
    target_compile_definitions(mpi_determinant PRIVATE
        ALGEBRA_ASSIGNMENT_SIZE=${ALGEBRA_ASSIGNMENT_SIZE}
    )
    if(ALGEBRA_ASSIGNMENT_BACKEND STREQUAL "cached")
        target_compile_definitions(mpi_determinant PRIVATE ALGEBRA_ASSIGNMENT_BENCHMARK_CACHED)
    endif()
    target_link_libraries(mpi_determinant PRIVATE
        MPI::MPI_CXX
        algebra::finite_fields
        algebra::matrices
    )
    target_include_directories(mpi_determinant PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
    )
endif()
