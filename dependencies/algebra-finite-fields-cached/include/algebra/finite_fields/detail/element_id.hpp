#ifndef ALGEBRA_FINITE_FIELDS_CACHED_DETAIL_ELEMENT_ID_HPP
#define ALGEBRA_FINITE_FIELDS_CACHED_DETAIL_ELEMENT_ID_HPP

#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace algebra::detail::cached_fields {

// ID conversion helpers; the agreed design and checklist are in docs/cache-design.md.
//
// OriginalField will be a template parameter. Tests can initially pass the
// existing algebra::FiniteField<11, 3>; the cached provider will later pass
// its privately named original field implementation.
//
// encode<OriginalField>(original_element):
//     id = 0
//     place_value = 1
//     for each coefficient degree below OriginalField.extension_degree:
//         digit = original_element.coefficient(degree).representative()
//         id += digit * place_value
//         place_value *= OriginalField.characteristic
//     return id

template <typename OriginalField>
std::uint32_t encode(const typename OriginalField::Element& original_element) {
    std::uint32_t id = 0;
    std::uint32_t place_value = 1;
    for (std::size_t degree = 0; degree < OriginalField::extension_degree; ++degree) {
        const auto digit = original_element.coefficient(degree).representative();
        id += digit * place_value;
        place_value *= OriginalField::characteristic;
    }
    if (id >= OriginalField::cardinality()) {
        throw std::logic_error("encode: computed id exceeds field cardinality");
    }
    return id;
}

//
// decode<OriginalField>(id):
//     validate id against OriginalField.cardinality()
//     return OriginalField.Element.from_index(id)
//
// Keep the definitions in this header because they will be templates.
// The element ID is an enumeration label, not an integer embedding.
template <typename OriginalField>
typename OriginalField::Element decode(std::uint32_t id) {
    if (id >= OriginalField::cardinality()) {
        throw std::out_of_range("decode: id exceeds field cardinality");
    }
    return OriginalField::Element::from_index(id);
}

}  // namespace algebra::detail::cached_fields

#endif  // ALGEBRA_FINITE_FIELDS_CACHED_DETAIL_ELEMENT_ID_HPP
