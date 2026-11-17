#ifndef ALGEBRA_FINITE_FIELDS_DETAIL_CACHE_STATE_HPP
#define ALGEBRA_FINITE_FIELDS_DETAIL_CACHE_STATE_HPP

#include <algebra/finite_fields/detail/element_id.hpp>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace algebra::detail::cached_fields {
// Singleton cache state for a given OriginalField
// Initialized once on first access, reused with state_for<OriginalField>() , destroyed at program exit
template <typename OriginalField>
class CacheState {
   private:
    // private sequence of OriginalField.Element values
    std::vector<typename OriginalField::Element> elements_;

   public:
    using OriginalFieldType = OriginalField;

    CacheState() {
        OriginalField::initialize();
        elements_.reserve(OriginalField::cardinality());

        for (std::uint32_t id = 0; id < OriginalField::cardinality(); ++id) {
            elements_.push_back(decode<OriginalField>(id));
        }
    }

    std::size_t size() const {
        return elements_.size();
    }

    const typename OriginalField::Element& value(std::uint32_t id) const {
        if (id >= elements_.size()) {
            throw std::out_of_range("CacheState: id exceeds field cardinality");
        }
        return elements_[id];
    }
};

template <typename OriginalField>
const CacheState<OriginalField>& state_for() {
    static const CacheState<OriginalField> instance;
    return instance;
}

}  // namespace algebra::detail::cached_fields

#endif  // ALGEBRA_FINITE_FIELDS_DETAIL_CACHE_STATE_HPP
