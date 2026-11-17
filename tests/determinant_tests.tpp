#ifdef ALGEBRA_TEST_CACHED_FIELDS
#include <algebra/finite_fields/detail/cached_finite_field.hpp>
#else
#include <algebra/finite_fields/finite_fields.hpp>
#endif
#include <exception>
#include <iostream>

#include "determinant.hpp"

namespace {

using Field = algebra::FiniteField<11, 3>;
using Element = Field::Element;
using Matrix = algebra::Matrix<Element, 2, 2>;

int check_determinant(const char* name, Matrix matrix, const Element& expected) {
    const Matrix before = matrix;
    const Element actual = algebra::determinant(matrix);
    int failures = 0;

    if (actual != expected) {
        std::cerr << "FAIL " << name << ": expected " << expected
                  << ", got " << actual << '\n';
        ++failures;
    }

    if (matrix != before) {
        std::cerr << "FAIL " << name << ": the input matrix changed\n";
        ++failures;
    }

    if (failures == 0) {
        std::cout << "PASS " << name << '\n';
    }

    return failures;
}

}  // namespace

int main() {
    try {
        Field::initialize();

        const Element zero = Element::zero();
        const Element one = Element::one();
        const Element two = Element::from_integer(2);
        const Element four = Element::from_integer(4);
        const Element negative_one = Element::from_integer(10);  // -1 modulo 11

        // Enumeration index 11 represents x; the integer 11 would represent 0.
        const Element x = Element::from_index(11);
        const Element negative_x = Element::from_coefficients({0, 10, 0});

        int failures = 0;

        failures += check_determinant("zero matrix", Matrix::zero(), zero);
        failures += check_determinant("identity matrix", Matrix::one(), one);

        // The second row is twice the first, so the determinant is zero.
        const Matrix singular{{one, two}, {two, four}};
        failures += check_determinant("singular matrix", singular, zero);

        // Selecting a nonzero pivot requires a row swap, which changes the sign.
        const Matrix row_swap{{zero, one}, {one, zero}};
        failures += check_determinant("row swap", row_swap, negative_one);

        // This case includes an element outside the prime subfield: det = -x.
        const Matrix extension{{zero, x}, {one, zero}};
        failures += check_determinant("extension field entry", extension, negative_x);

        // The first pivot is 2, exercising reciprocal-based elimination.
        // The independent 2-by-2 formula gives 2*1 - 1*1 = 1.
        const Matrix non_unit_pivot{{two, one}, {one, one}};
        failures += check_determinant("non-unit pivot", non_unit_pivot, one);

        // These two matrices already have their determinants checked above.
        // Now check that their product satisfies det(A*B) = det(A)*det(B).
        const Matrix product = non_unit_pivot.multiply(extension);
        const Element expected_product = algebra::determinant(non_unit_pivot)
                                             .multiply(algebra::determinant(extension));
        failures += check_determinant("determinant identity", product, expected_product);

        if (failures != 0) {
            std::cerr << failures << " check(s) failed\n";
            return 1;
        }

        std::cout << "All determinant tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Unexpected exception: " << error.what() << '\n';
        return 1;
    }
}
