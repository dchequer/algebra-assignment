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

    std::uint16_t result_id_;
    std::uint16_t EMPTY = 0xFFFF;  // 65535 - max value for uint16_t

    std::vector<std::uint16_t> multiplication_results;
    std::size_t multiplication_misses = 0;

    std::vector<std::uint16_t> addition_results;
    std::size_t addition_misses = 0;

    std::vector<std::uint16_t> negation_results;
    std::vector<std::uint16_t> reciprocal_results;
    std::size_t negation_misses = 0;
    std::size_t reciprocal_misses = 0;

   public:
    using OriginalFieldType = OriginalField;

    CacheState() {
        OriginalField::initialize();
        elements_.reserve(OriginalField::cardinality());

        for (std::uint32_t id = 0; id < OriginalField::cardinality(); ++id) {
            elements_.push_back(decode<OriginalField>(id));
        }

        const std::size_t Q = static_cast<std::size_t>(OriginalField::cardinality());
        multiplication_results.assign(Q * Q, EMPTY);

        addition_results.assign(Q * Q, EMPTY);

        negation_results.assign(Q, EMPTY);
        reciprocal_results.assign(Q, EMPTY);
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

    std::uint16_t multiply_ids(std::uint16_t id1, std::uint16_t id2) {
        const std::size_t Q = elements_.size();

        if (id1 >= Q || id2 >= Q) {
            throw std::out_of_range("CacheState: multiply_ids: id exceeds field cardinality");
        }

        std::size_t index = static_cast<std::size_t>(id1) * Q + id2;
        result_id_ = multiplication_results[index];

        // cache miss
        if (result_id_ == EMPTY) {
            const auto& element1 = elements_[id1];
            const auto& element2 = elements_[id2];
            const auto product = element1.multiply(element2);
            result_id_ = encode<OriginalField>(product);
            multiplication_results[index] = result_id_;
            ++multiplication_misses;
        }

        return result_id_;
    }

    std::size_t multiplication_miss_count() const {
        return multiplication_misses;
    }

    std::uint16_t add_ids(std::uint16_t id1, std::uint16_t id2) {
        const std::size_t Q = elements_.size();

        if (id1 >= Q || id2 >= Q) {
            throw std::out_of_range("CacheState: add_ids: id exceeds field cardinality");
        }

        std::size_t index = static_cast<std::size_t>(id1) * Q + id2;
        result_id_ = addition_results[index];

        // cache miss
        if (result_id_ == EMPTY) {
            const auto& element1 = elements_[id1];
            const auto& element2 = elements_[id2];
            const auto sum = element1.add(element2);
            result_id_ = encode<OriginalField>(sum);
            addition_results[index] = result_id_;
            ++addition_misses;
        }

        return result_id_;
    };

    std::size_t addition_miss_count() const {
        return addition_misses;
    }

    std::uint16_t negate_id(std::uint16_t id) {
        const std::size_t Q = elements_.size();

        if (id >= Q) {
            throw std::out_of_range("CacheState: negate_id: id exceeds field cardinality");
        }

        std::uint16_t result_id = negation_results[id];

        // cache miss
        if (result_id == EMPTY) {
            const auto& element = elements_[id];
            const auto negated = element.negate();
            result_id = encode<OriginalField>(negated);
            negation_results[id] = result_id;
            ++negation_misses;
        }

        return result_id;
    };

    std::uint16_t reciprocal_id(std::uint16_t id) {
        const std::size_t Q = elements_.size();

        if (id >= Q) {
            throw std::out_of_range("CacheState: reciprocal_id: id exceeds field cardinality");
        }

        std::uint16_t result_id = reciprocal_results[id];

        // cache miss
        if (result_id == EMPTY) {
            const auto& element = elements_[id];
            const auto reciprocal = element.reciprocal();
            result_id = encode<OriginalField>(reciprocal);
            reciprocal_results[id] = result_id;
            ++reciprocal_misses;
        }

        return result_id;
    };

    std::size_t negation_miss_count() const {
        return negation_misses;
    }

    std::size_t reciprocal_miss_count() const {
        return reciprocal_misses;
    }
};

template <typename OriginalField>
CacheState<OriginalField>& state_for() {
    static CacheState<OriginalField> instance;
    return instance;
}

}  // namespace algebra::detail::cached_fields

#endif  // ALGEBRA_FINITE_FIELDS_DETAIL_CACHE_STATE_HPP
