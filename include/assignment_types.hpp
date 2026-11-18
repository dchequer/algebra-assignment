#ifndef ALGEBRA_ASSIGNMENT_FIELD_HPP
#define ALGEBRA_ASSIGNMENT_FIELD_HPP

#include <algebra/finite_fields/finite_fields.hpp>
#include <cstddef>
#include <utility>
#include <vector>

namespace assignment {

using Field = algebra::FiniteField<11, 3>;
using Element = Field::Element;

using Row = std::vector<Element>;
using LocalRows = std::vector<std::pair<std::size_t, Row>>;
using Matrix = std::vector<Row>;

}  // namespace assignment

#endif  // ALGEBRA_ASSIGNMENT_FIELD_HPP
