#ifndef ALGEBRA_ASSIGNMENT_GENERATOR_HPP
#define ALGEBRA_ASSIGNMENT_GENERATOR_HPP

#include <algebra/matrices/matrices.hpp>
#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>

#include "assignment_types.hpp"

namespace matrix_generation {

using Field = assignment::Field;
using Element = assignment::Element;

inline std::uint32_t draw_coefficient(std::mt19937& generator, std::uint32_t prime) {
    if (prime < 2) {
        throw std::invalid_argument("The prime must be at least two");
    }
    constexpr std::uint64_t range = std::uint64_t{1} << 32;
    const std::uint64_t limit = range - range % prime;
    for (;;) {
        const std::uint64_t value = generator();
        if (value < limit) {
            return static_cast<std::uint32_t>(value % prime);
        }
    }
}

inline Element next_element(std::mt19937& generator) {
    Element::polynomial_type polynomial;
    for (std::size_t degree = 0; degree < Field::extension_degree; ++degree) {
        const auto coefficient = draw_coefficient(generator, Field::characteristic);
        polynomial.set_coefficient(degree, Element::coefficient_type::from_integer(coefficient));
    }
    return Element::from_polynomial(polynomial);
}

}  // namespace matrix_generation

#endif  // ALGEBRA_ASSIGNMENT_GENERATOR_HPP
