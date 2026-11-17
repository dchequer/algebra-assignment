#include <algebra/finite_fields/detail/cache_state.hpp>
#include <algebra/finite_fields/detail/original_finite_field.hpp>
#include <cstdint>
#include <exception>
#include <iostream>

namespace {

using InternalField = algebra::detail::original_fields::FiniteField<11, 3>;
using State = algebra::detail::cached_fields::CacheState<InternalField>;

}  // namespace

int main() {
    try {
        const State& state = algebra::detail::cached_fields::state_for<InternalField>();

        // verify that cache state size is 1331 (cardinality)
        if (state.size() != InternalField::cardinality()) {
            std::cerr << "FAIL: cache state size is " << state.size() << ", expected " << InternalField::cardinality() << "\n";
            return 1;
        }

        // verify repeated state access returns the same instance
        const State& state2 = algebra::detail::cached_fields::state_for<InternalField>();
        if (&state != &state2) {
            std::cerr << "FAIL: repeated state access returned different instances\n";
            return 1;
        }

        // verify that each element maps back to its index
        for (std::uint32_t id = 0; id < state.size(); ++id) {
            const InternalField::Element& element = state.value(id);
            const std::uint32_t encoded_id = algebra::detail::cached_fields::encode<InternalField>(element);
            if (encoded_id != id) {
                std::cerr << "FAIL: element at id " << id << " encoded to " << encoded_id << "\n";
                return 1;
            }
        }

        // verify that out-of-range access throws an exception
        try {
            state.value(state.size());
            std::cerr << "FAIL: out-of-range access did not throw an exception\n";
            return 1;
        } catch (const std::out_of_range&) {
            // expected
        } catch (const std::exception& error) {
            std::cerr << "FAIL: out-of-range access threw unexpected exception: " << error.what() << "\n";
            return 1;
        }

    } catch (const std::exception& error) {
        std::cerr << "FAIL: unexpected exception: " << error.what() << "\n";
        return 1;
    }
}