#include <algebra/matrices/matrices.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <random>
#include <utility>

#include "assignment_types.hpp"
#include "benchmark_cache.hpp"
#include "determinant.hpp"
#include "generator.hpp"

namespace {

using Field = assignment::Field;
using Element = assignment::Element;
constexpr std::size_t dimension = ALGEBRA_ASSIGNMENT_SIZE;
constexpr std::uint32_t seed = 123456u;
using Matrix = algebra::Matrix<Element, dimension, dimension>;
using Clock = std::chrono::steady_clock;

void populate(Matrix& matrix) {
    std::mt19937 generator(seed);
    for (std::size_t r = 0; r < dimension; ++r) {
        for (std::size_t c = 0; c < dimension; ++c) {
            matrix(r, c) = matrix_generation::next_element(generator);
            (void)matrix_generation::next_element(generator);  // Match the B draw in main.cpp.
        }
    }
}

}  // namespace

int main() {
    try {
        Field::initialize();
        std::cout << std::fixed << std::setprecision(6)
                  << "Matrix dimension " << dimension << '\n'
                  << "Seed " << seed << '\n'
                  << "Field order " << Field::cardinality() << '\n'
                  << "Modulus " << Field::modulus() << '\n' << std::flush;

        const auto cache_start = Clock::now();
        benchmark_cache::initialize();
        const double cache_seconds = std::chrono::duration<double>(Clock::now() - cache_start).count();
        std::cout << "Cache initialization seconds " << cache_seconds << '\n'
                  << "Generating A\n" << std::flush;

        const auto setup_start = Clock::now();
        Matrix matrix;
        populate(matrix);
        const double setup_seconds = std::chrono::duration<double>(Clock::now() - setup_start).count();
        std::cout << "Matrix preparation seconds " << setup_seconds << '\n';

#ifdef ALGEBRA_ASSIGNMENT_BENCHMARK_CACHED
        const bool cache_empty = benchmark_cache::arithmetic_entries_empty();
        std::cout << "Arithmetic cache entries empty before determinant " << std::boolalpha << cache_empty << '\n';
        if (!cache_empty) {
            std::cerr << "Matrix setup filled arithmetic cache entries; benchmark policy not met\n";
            return 1;
        }
#endif

        std::cout << "Computing det(A) sequentially\n"
                  << std::flush;
        const auto start = Clock::now();
        const Element result = algebra::determinant(std::move(matrix));
        const double seconds = std::chrono::duration<double>(Clock::now() - start).count();

        std::cout << "det(A) = " << result << '\n'
                  << "Coefficients [";
        for (std::size_t degree = 0; degree < Field::extension_degree; ++degree) {
            if (degree != 0) {
                std::cout << ", ";
            }
            std::cout << result.coefficient(degree);
        }
        std::cout << "]\nSequential determinant seconds " << seconds << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
