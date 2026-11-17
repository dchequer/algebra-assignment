#ifndef ALGEBRA_ASSIGNMENT_DETERMINANT_HPP
#define ALGEBRA_ASSIGNMENT_DETERMINANT_HPP

#include <algebra/matrices/matrices.hpp>

namespace algebra {

template <typename T, std::size_t dimension>
T determinant(Matrix<T, dimension, dimension> matrix) {
    T result = T::one();
    for (std::size_t k = 0; k < dimension; ++k) {
        std::size_t pivot_row = k;
        while (pivot_row < dimension && matrix(pivot_row, k).is_zero()) {
            ++pivot_row;
        }
        if (pivot_row == dimension) {
            return T::zero();
        }
        if (pivot_row != k) {
            matrix.swap_rows(pivot_row, k);
            result = result.negate();
        }
        const T pivot = matrix(k, k);
        result = result.multiply(pivot);
        T inverse = T::one();
        if (!pivot.is_one()) {
            inverse = pivot.reciprocal();
        }
        for (std::size_t i = k + 1; i < dimension; ++i) {
            if (matrix(i, k).is_zero()) {
                continue;
            }
            const T multiplier = matrix(i, k).multiply(inverse);
            matrix(i, k) = T::zero();
            for (std::size_t j = k + 1; j < dimension; ++j) {
                const T correction = multiplier.multiply(matrix(k, j));
                matrix(i, j) = matrix(i, j).add(correction.negate());
            }
        }
    }
    return result;
}

}  // namespace algebra

#endif  // ALGEBRA_ASSIGNMENT_DETERMINANT_HPP