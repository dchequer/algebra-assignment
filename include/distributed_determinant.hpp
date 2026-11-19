#ifndef ALGEBRA_ASSIGNMENT_DISTRIBUTED_DETERMINANT_HPP
#define ALGEBRA_ASSIGNMENT_DISTRIBUTED_DETERMINANT_HPP

#include <mpi.h>

#include <algebra/finite_fields/detail/element_id.hpp>
#include <algebra/finite_fields/finite_fields.hpp>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <utility>
#include <vector>

#include "assignment_types.hpp"

namespace algebra::distributed {

using Field = assignment::Field;
using Element = assignment::Element;
using Row = assignment::Row;
using LocalRows = assignment::LocalRows;

#ifdef ALGEBRA_ASSIGNMENT_MPI_PROFILE
struct Profile {
    double pivot_seconds = 0;
    double swap_seconds = 0;
    double broadcast_seconds = 0;
    double elimination_seconds = 0;
    std::size_t pivots = 0;
};
#endif

namespace detail {

inline int find_pivot(const LocalRows& local_rows, std::size_t column, std::size_t dimension) {
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

inline void swap_distributed_rows(LocalRows& local_rows, std::size_t first_row, std::size_t second_row, int rank, int size, std::size_t dimension) {
    // Pseudocode:
    //
    // If first_row == second_row, return: there is nothing to swap
    //
    // first_owner  = first_row % size
    // second_owner = second_row % size
    //
    // If both rows have the same owner:
    //     Only that owner finds both rows in local_rows
    //     Swap their Row contents, keeping global row numbers unchanged
    //
    // Otherwise:
    //     Only the two owner ranks participate:
    //         Find the row this rank owns.
    //         Encode its Elements into a uint16_t send buffer
    //         Exchange buffers with the other owner using MPI_Sendrecv
    //         Decode the received IDs into the existing Row
    //
    // Other ranks do nothing

    if (first_row == second_row) {
        return;
    }

    const int first_owner = static_cast<int>(first_row % size);
    const int second_owner = static_cast<int>(second_row % size);

    if (first_owner == second_owner) {
        if (rank == first_owner) {
            Row* first_ptr = nullptr;
            Row* second_ptr = nullptr;
            for (auto& [global_row, row] : local_rows) {
                if (global_row == first_row) {
                    first_ptr = &row;
                } else if (global_row == second_row) {
                    second_ptr = &row;
                }
            }
            if (!first_ptr || !second_ptr) {
                throw std::logic_error("swap_distributed_rows: owner rank does not have both rows");
            }
            std::swap(*first_ptr, *second_ptr);
        }
    } else {
        if (rank == first_owner || rank == second_owner) {
            std::vector<uint16_t> send_buffer(dimension);
            std::vector<uint16_t> recv_buffer(dimension);

            // find the row this rank owns
            Row* owned_row = nullptr;
            for (auto& [global_row, row] : local_rows) {
                if ((rank == first_owner && global_row == first_row) ||
                    (rank == second_owner && global_row == second_row)) {
                    owned_row = &row;
                    break;
                }
            }
            if (!owned_row) {
                throw std::logic_error("swap_distributed_rows: owner rank does not have its owned row");
            }

            // encode the Elements into send_buffer
            for (std::size_t col = 0; col < dimension; ++col) {
                send_buffer[col] = static_cast<std::uint16_t>(algebra::detail::cached_fields::encode<Field>((*owned_row)[col]));
            }

            // exchange buffers with the other owner using MPI_Sendrecv
            MPI_Sendrecv(
                send_buffer.data(), static_cast<int>(dimension), MPI_UINT16_T, (rank == first_owner ? second_owner : first_owner), 0,
                recv_buffer.data(), static_cast<int>(dimension), MPI_UINT16_T, (rank == first_owner ? second_owner : first_owner), 0,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // decode the received IDs into the existing Row
            for (std::size_t col = 0; col < dimension; ++col) {
                (*owned_row)[col] = Element::from_index(recv_buffer[col]);
            }
        }
    }
}

inline Row broadcast_pivot_row(
    const LocalRows& local_rows,
    std::size_t pivot_position,
    int rank,
    int size,
    std::size_t dimension) {
    // owner = pivot_position % size
    const int owner = static_cast<int>(pivot_position % size);

    // Create a uint16_t buffer with dimension entries on EVERY rank
    std::vector<uint16_t> buffer(dimension);

    // If this rank is the owner:
    //     Find pivot_position in local_rows
    //     Encode each Element in that row into the buffer
    if (rank == owner) {
        const Row* pivot_row_ptr = nullptr;
        for (const auto& [global_row, row] : local_rows) {
            if (global_row == pivot_position) {
                pivot_row_ptr = &row;
                break;
            }
        }
        if (!pivot_row_ptr) {
            throw std::logic_error("broadcast_pivot_row: owner rank does not have the pivot row");
        }
        for (std::size_t col = 0; col < dimension; ++col) {
            buffer[col] = static_cast<std::uint16_t>(algebra::detail::cached_fields::encode<Field>((*pivot_row_ptr)[col]));
        }
    }

    // Every rank calls MPI_Bcast on the buffer
    // The owner is the broadcast root
    MPI_Bcast(
        buffer.data(),
        static_cast<int>(dimension),
        MPI_UINT16_T,
        owner,
        MPI_COMM_WORLD);

    // On every rank:
    //     Decode the received IDs into a Row of Elements
    //     Return that Row
    Row pivot_row;
    for (std::size_t col = 0; col < dimension; ++col) {
        pivot_row.push_back(Element::from_index(buffer[col]));
    }
    return pivot_row;
}

inline void eliminate_local_rows(LocalRows& local_rows, const Row& pivot_row, std::size_t pivot_position, std::size_t dimension) {
    // For each (global_row, row) in local_rows:
    //     If global_row > pivot_position and row[pivot_position] is nonzero:
    //         Compute multiplier = row[pivot_position] / pivot_row[pivot_position]
    //         Set row[pivot_position] = 0
    //         For each column j > pivot_position:
    //             row[j] = row[j] - multiplier * pivot_row[j]
    const Element pivot_reciprocal = pivot_row[pivot_position].reciprocal();

    for (auto& [global_row, row] : local_rows) {
        if (global_row > pivot_position && !row[pivot_position].is_zero()) {
            const Element multiplier = row[pivot_position].multiply(pivot_reciprocal);
            row[pivot_position] = Element::zero();
            for (std::size_t j = pivot_position + 1; j < dimension; ++j) {
                const Element correction = multiplier.multiply(pivot_row[j]);
                row[j] = row[j].add(correction.negate());
            }
        }
    }
}

}  // namespace detail

inline Element distributed_determinant(LocalRows& local_rows, std::size_t dimension, int rank, int size
#ifdef ALGEBRA_ASSIGNMENT_MPI_PROFILE
                                       , Profile& profile
#endif
                                       ) {
    Element result = Element::one();

    for (std::size_t k = 0; k < dimension; ++k) {
#ifdef ALGEBRA_ASSIGNMENT_MPI_PROFILE
        double phase_start = MPI_Wtime();
#endif
        const int pivot = detail::find_pivot(local_rows, k, dimension);
#ifdef ALGEBRA_ASSIGNMENT_MPI_PROFILE
        profile.pivot_seconds += MPI_Wtime() - phase_start;
#endif

        if (pivot == static_cast<int>(dimension)) {
            // std::printf("Rank %d | No pivot found for column %zu\n", rank, k);
            return Element::zero();
        }

#ifdef ALGEBRA_ASSIGNMENT_MPI_PROFILE
        ++profile.pivots;
        phase_start = MPI_Wtime();
#endif
        if (pivot != static_cast<int>(k)) {
            // std::printf("Rank %d | Swapping global row %zu with global row %d\n", rank, k, pivot);
            detail::swap_distributed_rows(local_rows, k, static_cast<std::size_t>(pivot), rank, size, dimension);
            result = result.negate();
        }

#ifdef ALGEBRA_ASSIGNMENT_MPI_PROFILE
        profile.swap_seconds += MPI_Wtime() - phase_start;
        phase_start = MPI_Wtime();
#endif
        const Row pivot_row = detail::broadcast_pivot_row(local_rows, k, rank, size, dimension);
#ifdef ALGEBRA_ASSIGNMENT_MPI_PROFILE
        profile.broadcast_seconds += MPI_Wtime() - phase_start;
#endif
        result = result.multiply(pivot_row[k]);
#ifdef ALGEBRA_ASSIGNMENT_MPI_PROFILE
        phase_start = MPI_Wtime();
#endif
        detail::eliminate_local_rows(local_rows, pivot_row, k, dimension);
#ifdef ALGEBRA_ASSIGNMENT_MPI_PROFILE
        profile.elimination_seconds += MPI_Wtime() - phase_start;
#endif
    }

    return result;
}

}  // namespace algebra::distributed

#endif  // ALGEBRA_ASSIGNMENT_DISTRIBUTED_DETERMINANT_HPP
