#ifndef ALGEBRA_ASSIGNMENT_BENCHMARK_CACHE_HPP
#define ALGEBRA_ASSIGNMENT_BENCHMARK_CACHE_HPP

#include "assignment_types.hpp"

#ifdef ALGEBRA_ASSIGNMENT_BENCHMARK_CACHED
#include <algebra/finite_fields/detail/cache_state.hpp>
#include <algebra/finite_fields/detail/original_finite_field.hpp>
#endif

namespace benchmark_cache {

inline void initialize() {
#ifdef ALGEBRA_ASSIGNMENT_BENCHMARK_CACHED
    (void)assignment::Element::zero().coefficient(0);
#endif
}

inline bool arithmetic_entries_empty() {
#ifdef ALGEBRA_ASSIGNMENT_BENCHMARK_CACHED
    using OriginalField = algebra::detail::original_fields::FiniteField<
        assignment::Field::characteristic, assignment::Field::extension_degree>;
    const auto& state = algebra::detail::cached_fields::state_for<OriginalField>();
    return state.multiplication_miss_count() == 0 &&
           state.addition_miss_count() == 0 &&
           state.negation_miss_count() == 0 &&
           state.reciprocal_miss_count() == 0;
#else
    return true;
#endif
}

}  // namespace benchmark_cache

#endif  // ALGEBRA_ASSIGNMENT_BENCHMARK_CACHE_HPP
