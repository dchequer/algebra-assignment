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
#include <random>
#include <utility>
#include <vector>

#include "assignment_types.hpp"
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

    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);
    // printf("Hello from process %d of %d on %s\n", rank, size, processor_name);

    LocalRows local_rows;
    Element sequential_result = Element::zero();

    if (rank == 0) {
        std::cout << std::fixed << std::setprecision(6)
                  << "Matrix dimension " << dimension << '\n'
                  << "Seed " << seed << '\n'
                  << "Field order " << Field::cardinality() << '\n'
                  << "Modulus " << Field::modulus() << '\n'
                  << "MPI ranks " << size << '\n'
                  << "Generating A and checking sequential determinant\n"
                  << std::flush;

        Matrix matrix(dimension, Row(dimension));
        populate(matrix);

        algebra::Matrix<Element, dimension, dimension> reference_matrix;
        for (std::size_t r = 0; r < dimension; ++r) {
            for (std::size_t c = 0; c < dimension; ++c) {
                reference_matrix(r, c) = matrix[r][c];
            }
        }

        sequential_result = algebra::determinant(reference_matrix);

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
    if (rank == 0) {
        std::cout << "Computing det(A) with MPI\n" << std::flush;
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

    // for (const auto& [global_row, row] : local_rows) {
    //     std::printf("Rank %d | Global Row %zu:", rank, global_row);
    //     for (const auto& element : row) {
    //         std::printf(" %u", static_cast<std::uint16_t>(algebra::detail::cached_fields::encode<Field>(element)));
    //     }
    //     std::printf("\n");
    // }

    MPI_Finalize();
    return EXIT_SUCCESS;
}
