#include <mpi.h>

#include <algebra/finite_fields/detail/element_id.hpp>
#include <algebra/finite_fields/finite_fields.hpp>
#include <algebra/matrices/matrices.hpp>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <random>
#include <utility>
#include <vector>

#include "assignment_types.hpp"
#include "benchmark_cache.hpp"
#include "determinant.hpp"
#include "distributed_determinant.hpp"
#include "generator.hpp"

namespace {

using Field = algebra::distributed::Field;
using Element = algebra::distributed::Element;
constexpr std::size_t dimension = ALGEBRA_ASSIGNMENT_SIZE;
constexpr std::uint32_t seed = 123456u;

using Row = assignment::Row;
using Matrix = assignment::Matrix;
using LocalRows = assignment::LocalRows;

void populate(Matrix& m) {
    std::mt19937 generator(seed);
    for (std::size_t r = 0; r < dimension; ++r) {
        for (std::size_t c = 0; c < dimension; ++c) {
            m[r][c] = matrix_generation::next_element(generator);
            (void)matrix_generation::next_element(generator);  // discard
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    Field::initialize();
    MPI_Barrier(MPI_COMM_WORLD);
    const double cache_start = MPI_Wtime();
    benchmark_cache::initialize();
    const double local_cache_seconds = MPI_Wtime() - cache_start;
    double max_cache_seconds = 0.0;
    MPI_Reduce(&local_cache_seconds, &max_cache_seconds, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);
    // printf("Hello from process %d of %d on %s\n", rank, size, processor_name);

    LocalRows local_rows;
    using ReferenceMatrix = algebra::Matrix<Element, dimension, dimension>;
    std::optional<ReferenceMatrix> reference_matrix;

    if (rank == 0) {
        std::cout << std::fixed << std::setprecision(6)
                  << "Matrix dimension " << dimension << '\n'
                  << "Seed " << seed << '\n'
                  << "Field order " << Field::cardinality() << '\n'
                  << "Modulus " << Field::modulus() << '\n'
                  << "MPI ranks " << size << '\n'
                  << "Cache initialization max seconds " << max_cache_seconds << '\n'
                  << "Generating A\n"
                  << std::flush;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    const double setup_start = MPI_Wtime();
    if (rank == 0) {
        Matrix matrix(dimension, Row(dimension));
        populate(matrix);

        reference_matrix.emplace();
        for (std::size_t r = 0; r < dimension; ++r) {
            for (std::size_t c = 0; c < dimension; ++c) {
                (*reference_matrix)(r, c) = matrix[r][c];
            }
        }

        // distribute rows cyclically (row % size)
        for (std::size_t global_row = 0; global_row < dimension; ++global_row) {
            const int owner = static_cast<int>(global_row % size);

            if (owner == 0) {
                local_rows.emplace_back(global_row, matrix[global_row]);
            } else {
                std::vector<uint16_t> encoded_row;
                for (const auto& element : matrix[global_row]) {
                    const auto id = static_cast<std::uint16_t>(algebra::detail::cached_fields::encode<Field>(element));
                    encoded_row.push_back(id);
                }
                MPI_Send(
                    encoded_row.data(),
                    static_cast<int>(dimension),
                    MPI_UINT16_T,
                    owner,
                    static_cast<int>(global_row),
                    MPI_COMM_WORLD);
            }
        }
    } else {
        // receive assigned rows in cyclic order
        for (std::size_t global_row = static_cast<std::size_t>(rank); global_row < dimension; global_row += size) {
            std::vector<uint16_t> encoded_row(dimension);

            MPI_Recv(
                encoded_row.data(),
                static_cast<int>(dimension),
                MPI_UINT16_T,
                0,
                static_cast<int>(global_row),
                MPI_COMM_WORLD,
                MPI_STATUS_IGNORE);

            Row decoded_row;
            for (const auto& id : encoded_row) {
                const auto element = Element::from_index(id);
                decoded_row.push_back(element);
            }
            local_rows.emplace_back(global_row, std::move(decoded_row));
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    const double local_setup_seconds = MPI_Wtime() - setup_start;
    double max_setup_seconds = 0.0;
    MPI_Reduce(&local_setup_seconds, &max_setup_seconds, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

#ifdef ALGEBRA_ASSIGNMENT_BENCHMARK_CACHED
    const int local_cache_empty = benchmark_cache::arithmetic_entries_empty() ? 1 : 0;
    int all_caches_empty = 0;
    MPI_Allreduce(&local_cache_empty, &all_caches_empty, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    if (rank == 0) {
        std::cout << "Arithmetic cache entries empty on all ranks before determinant "
                  << std::boolalpha << (all_caches_empty != 0) << '\n';
    }
    if (!all_caches_empty) {
        if (rank == 0) {
            std::cerr << "Matrix setup filled arithmetic cache entries; benchmark policy not met\n";
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }
#endif
    if (rank == 0) {
        std::cout << "Matrix preparation and distribution max seconds " << max_setup_seconds << '\n'
                  << "Computing det(A) with MPI\n"
                  << std::flush;
    }
    MPI_Barrier(MPI_COMM_WORLD);
    const double start = MPI_Wtime();

    const Element result = algebra::distributed::distributed_determinant(local_rows, dimension, rank, size);

    const double elapsed = MPI_Wtime() - start;
    double max_elapsed;
    MPI_Reduce(
        &elapsed,
        &max_elapsed,
        1,
        MPI_DOUBLE,
        MPI_MAX,
        0,
        MPI_COMM_WORLD);

    if (rank == 0) {
        const Element sequential_result = algebra::determinant(*reference_matrix);
        std::cout << "det(A) = " << result << '\n'
                  << "Coefficients [";
        for (std::size_t degree = 0; degree < Field::extension_degree; ++degree) {
            if (degree != 0) {
                std::cout << ", ";
            }
            std::cout << result.coefficient(degree);
        }
        std::cout << "]\nMPI determinant seconds " << max_elapsed << '\n'
                  << "Sequential determinant = " << sequential_result << '\n'
                  << "Determinants match " << std::boolalpha << (result == sequential_result) << '\n';
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}
