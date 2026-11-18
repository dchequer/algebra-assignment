#include <algebra/finite_fields/finite_fields.hpp>
#include <algebra/matrices/matrices.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>

#include "assignment_types.hpp"
#include "determinant.hpp"
#include "generator.hpp"

namespace {

using Field = assignment::Field;
using Element = assignment::Element;
constexpr std::size_t dimension = ALGEBRA_ASSIGNMENT_SIZE;
constexpr std::uint32_t seed = 123456u;
using Matrix = algebra::Matrix<Element, dimension, dimension>;
using Clock = std::chrono::steady_clock;

static_assert(algebra::FieldElement<Element>);
static_assert(algebra::RingElement<Matrix>);

double seconds(Clock::time_point start) {
    return std::chrono::duration<double>(Clock::now() - start).count();
}

void populate(Matrix& a, Matrix& b) {
    std::mt19937 generator(seed);
    for (std::size_t i = 0; i < dimension; ++i) {
        for (std::size_t j = 0; j < dimension; ++j) {
            a(i, j) = matrix_generation::next_element(generator);
            b(i, j) = matrix_generation::next_element(generator);
        }
    }
}

void print_determinant(const char* name, const Element& value, double elapsed) {
    std::cout << name << " = " << value << '\n';
    std::cout << "Coefficients [";
    for (std::size_t degree = 0; degree < Field::extension_degree; ++degree) {
        if (degree != 0) {
            std::cout << ", ";
        }
        std::cout << value.coefficient(degree);
    }
    std::cout << "]\nSeconds " << elapsed << '\n'
              << std::flush;
}

}  // namespace

int main() {
    try {
        const auto total_start = Clock::now();
        Field::initialize();
        std::cout << std::fixed << std::setprecision(6)
                  << "Matrix dimension " << dimension << '\n'
                  << "Seed " << seed << '\n'
                  << "Field order " << Field::cardinality() << '\n'
                  << "Modulus " << Field::modulus() << '\n';

        std::cout << "Generating A and B\n"
                  << std::flush;
        auto start = Clock::now();
        Matrix a;
        Matrix b;
        populate(a, b);
        std::cout << "Generation seconds " << seconds(start) << '\n';

        std::cout << "Computing C = A * B\n"
                  << std::flush;
        start = Clock::now();
        const Matrix c = a.multiply(b);
        std::cout << "Product seconds " << seconds(start) << '\n';

        std::cout << "Computing det(A)\n"
                  << std::flush;
        start = Clock::now();
        const auto det_a = determinant(a);
        print_determinant("det(A)", det_a, seconds(start));

        std::cout << "Computing det(B)\n"
                  << std::flush;
        start = Clock::now();
        const auto det_b = determinant(b);
        print_determinant("det(B)", det_b, seconds(start));

        std::cout << "Computing det(C)\n"
                  << std::flush;
        start = Clock::now();
        const auto det_c = determinant(c);
        print_determinant("det(C)", det_c, seconds(start));

        const auto expected = det_a.multiply(det_b);
        const bool verified = det_c == expected;
        std::cout << "det(A) * det(B) = " << expected << '\n'
                  << "Identity verified " << std::boolalpha << verified << '\n'
                  << "Total seconds " << seconds(total_start) << '\n';
        return verified ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
