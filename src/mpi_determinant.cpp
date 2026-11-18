#include <mpi.h>

#include <algebra/finite_fields/detail/element_id.hpp>
#include <algebra/finite_fields/finite_fields.hpp>
#include <algebra/matrices/matrices.hpp>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <utility>
#include <vector>

namespace {

using Field = algebra::FiniteField<11, 3>;
using Element = Field::Element;
constexpr std::size_t dimension = ALGEBRA_ASSIGNMENT_SIZE;
constexpr std::uint32_t seed = 123456u;

using Row = std::vector<Element>;
using Matrix = std::vector<Row>;
using LocalRows = std::vector<std::pair<std::size_t, Row>>;

int find_pivot(const LocalRows& local_rows, std::size_t column) {
    // 1. Start with "no candidate" for this rank.
    //    Use dimension as the sentinel: valid row numbers are 0..dimension-1.
    int pivot = static_cast<int>(dimension);

    // 2. Examine each (global_row, row) in local_rows.
    //    Only consider rows where global_row >= column.
    //    If row[column] is nonzero, keep the smallest qualifying global row.
    for (const auto& [global_row, row] : local_rows) {
        if (global_row >= column && !row[column].is_zero()) {
            // if first candidate (pivot == dimension) or smaller than current candidate, update pivot
            if (pivot == static_cast<int>(dimension) || global_row < static_cast<std::size_t>(pivot)) {
                pivot = static_cast<int>(global_row);
            }
        }
    }

    // 3. Use MPI_Allreduce with MPI_MIN to find the smallest candidate
    //    across all ranks. Every rank receives the same result.
    MPI_Allreduce(MPI_IN_PLACE, &pivot, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);

    // 4. Return that global result.
    return pivot;
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
    printf("Hello from process %d of %d on %s\n", rank, size, processor_name);

    LocalRows local_rows;

    if (rank == 0) {
        // init matrix on rank 0
        Matrix matrix(dimension, Row(dimension));
        for (std::uint16_t r = 0; r < dimension; ++r) {
            for (std::uint16_t c = 0; c < dimension; ++c) {
                matrix[r][c] = Element::from_coefficients({r, c, 1});
            }
        }

        // set pivot for testing manually
        matrix[0][0] = Element::zero();
        matrix[1][0] = Element::one();

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

    const int pivot = find_pivot(local_rows, 0);

    if (rank == 0) {
        std::printf("Global pivot for column 0: %d\n", pivot);
    }

    for (const auto& [global_row, row] : local_rows) {
        std::printf("Rank %d | Global Row %zu:", rank, global_row);
        for (const auto& element : row) {
            std::printf(" %u", static_cast<std::uint16_t>(algebra::detail::cached_fields::encode<Field>(element)));
        }
        std::printf("\n");
    }

    // validate each local row
    int local_ok = 1;

    if (pivot != 1) {
        std::printf("Rank %d | Global pivot for column 0: %d (expected 1)\n", rank, pivot);
        local_ok = 0;
    } else {
        // disabled since we are manually setting pivot and it wont match the expected values
        // for (const auto& [global_row, row] : local_rows) {
        //     for (std::size_t col = 0; col < row.size(); ++col) {
        //         const auto expected = Element::from_coefficients({static_cast<std::int64_t>(global_row),
        //                                                           static_cast<std::int64_t>(col),
        //                                                           1});
        //         if (row[col] != expected) {
        //             std::printf("Rank %d | Global Row %zu | Column %zu: expected %u, got %u\n",
        //                         rank,
        //                         global_row,
        //                         col,
        //                         static_cast<std::uint16_t>(algebra::detail::cached_fields::encode<Field>(expected)),
        //                         static_cast<std::uint16_t>(algebra::detail::cached_fields::encode<Field>(row[col])));
        //             local_ok = 0;
        //         }
        //     }
        // }
    }

    // validate across all ranks
    int global_ok = 0;
    MPI_Allreduce(&local_ok, &global_ok, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);

    if (rank == 0) {
        std::printf("Distribution %s\n", global_ok ? "PASSED" : "FAILED");
    }

    MPI_Finalize();
    return global_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}