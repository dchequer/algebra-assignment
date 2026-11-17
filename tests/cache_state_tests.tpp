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
        State& state = algebra::detail::cached_fields::state_for<InternalField>();

        // verify that cache state size is 1331 (cardinality)
        if (state.size() != InternalField::cardinality()) {
            std::cerr << "FAIL: cache state size is " << state.size() << ", expected " << InternalField::cardinality() << "\n";
            return 1;
        }

        // verify repeated state access returns the same instance
        State& state2 = algebra::detail::cached_fields::state_for<InternalField>();
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

        // VERIFY MULTIPLICATION CACHING
        // request IDs (2, 11): 2 * x = 2x, which has coefficients {0, 2, 0} and should encode to id 22
        const std::uint16_t id1 = 2;                   // corresponds to element 2
        const std::uint16_t id2 = 11;                  // corresponds to element x
        const std::uint16_t expected_product_id = 22;  // corresponds to element 2x
        const std::uint16_t product_id = state.multiply_ids(id1, id2);
        if (product_id != expected_product_id) {
            std::cerr << "FAIL: multiply_ids(" << id1 << ", " << id2 << ") returned " << product_id << ", expected " << expected_product_id << "\n";
            return 1;
        }
        if (state.multiplication_miss_count() != 1) {
            std::cerr << "FAIL: multiplication miss count is " << state.multiplication_miss_count() << ", expected 1\n";
            return 1;
        }
        // repeat the multiplication to check that it hits the cache
        const std::uint16_t product_id_cached = state.multiply_ids(id1, id2);
        if (product_id_cached != expected_product_id) {
            std::cerr << "FAIL: multiply_ids(" << id1 << ", " << id2 << ") returned " << product_id_cached << " on second call, expected " << expected_product_id << "\n";
            return 1;
        }
        if (state.multiplication_miss_count() != 1) {
            std::cerr << "FAIL: multiplication miss count is " << state.multiplication_miss_count() << " after second call, expected 1\n";
            return 1;
        }

        // request IDs (0, 11): 0 * x = 0, which has coefficients {0, 0, 0} and should encode to id 0
        const std::uint16_t id3 = 0;                   // corresponds to element 0
        const std::uint16_t id4 = 11;                  // corresponds to element x
        const std::uint16_t expected_product_id2 = 0;  // corresponds to element 0
        const std::uint16_t product_id2 = state.multiply_ids(id3, id4);
        if (product_id2 != expected_product_id2) {
            std::cerr << "FAIL: multiply_ids(" << id3 << ", " << id4 << ") returned " << product_id2 << ", expected " << expected_product_id2 << "\n";
            return 1;
        }
        if (state.multiplication_miss_count() != 2) {
            std::cerr << "FAIL: multiplication miss count is " << state.multiplication_miss_count() << ", expected 2\n";
            return 1;
        }
        // repeat the multiplication to check that it hits the cache
        const std::uint16_t product_id2_cached = state.multiply_ids(id3, id4);
        if (product_id2_cached != expected_product_id2) {
            std::cerr << "FAIL: multiply_ids(" << id3 << ", " << id4 << ") returned " << product_id2_cached << " on second call, expected " << expected_product_id2 << "\n";
            return 1;
        }
        if (state.multiplication_miss_count() != 2) {
            std::cerr << "FAIL: multiplication miss count is " << state.multiplication_miss_count() << " after second call, expected 2\n";
            return 1;
        }

        // request IDs (121, 11): x^2 * x = x^3, which has coefficients {7, 10, 0} and should encode to id 117
        // x^3 reduces modulo x^3 + x + 4 to -x - 4
        // modulo 11, this is 7 + 10*x: {7, 10, 0} -> ID = 7*1 + 10*11 + 0*121 = 7 + 110 = 117
        const std::uint16_t id5 = 121;                   // corresponds to element x^2
        const std::uint16_t id6 = 11;                    // corresponds to element x
        const std::uint16_t expected_product_id3 = 117;  // corresponds to element -x + 4
        const std::uint16_t product_id3 = state.multiply_ids(id5, id6);
        if (product_id3 != expected_product_id3) {
            std::cerr << "FAIL: multiply_ids(" << id5 << ", " << id6 << ") returned " << product_id3 << ", expected " << expected_product_id3 << "\n";
            return 1;
        }
        if (state.multiplication_miss_count() != 3) {
            std::cerr << "FAIL: multiplication miss count is " << state.multiplication_miss_count() << ", expected 3\n";
            return 1;
        }

        // verify that out-of-range multiplication IDs throw an exception
        try {
            state.multiply_ids(state.size(), 0);
            std::cerr << "FAIL: out-of-range multiply_ids did not throw an exception\n";
            return 1;
        } catch (const std::out_of_range&) {
            // expected
        } catch (const std::exception& error) {
            std::cerr << "FAIL: out-of-range multiply_ids threw unexpected exception: " << error.what() << "\n";
            return 1;
        }

        // VERIFY ADDITION CACHING
        // request IDs (2, 11): 2 + x, which has coefficients {2, 1, 0} and should encode to id 13
        const std::uint16_t expected_sum_id = 13;  // corresponds to element 2 + x
        const std::uint16_t sum_id = state.add_ids(id1, id2);
        if (sum_id != expected_sum_id) {
            std::cerr << "FAIL: add_ids(" << id1 << ", " << id2 << ") returned " << sum_id << ", expected " << expected_sum_id << "\n";
            return 1;
        }
        if (state.addition_miss_count() != 1) {
            std::cerr << "FAIL: addition miss count is " << state.addition_miss_count() << ", expected 1\n";
            return 1;
        }
        // repeat the addition to check that it hits the cache
        const std::uint16_t sum_id_cached = state.add_ids(id1, id2);
        if (sum_id_cached != expected_sum_id) {
            std::cerr << "FAIL: add_ids(" << id1 << ", " << id2 << ") returned " << sum_id_cached << " on second call, expected " << expected_sum_id << "\n";
            return 1;
        }
        if (state.addition_miss_count() != 1) {
            std::cerr << "FAIL: addition miss count is " << state.addition_miss_count() << " after second call, expected 1\n";
            return 1;
        }

        // request IDs (10, 1): 10 + 1 = 0 (mod 11), which has coefficients {0, 0, 0} and should encode to id 0
        const std::uint16_t id7 = 10;              // corresponds to element 10
        const std::uint16_t id8 = 1;               // corresponds to element 1
        const std::uint16_t expected_sum_id2 = 0;  // corresponds to element 0
        const std::uint16_t sum_id2 = state.add_ids(id7, id8);
        if (sum_id2 != expected_sum_id2) {
            std::cerr << "FAIL: add_ids(" << id7 << ", " << id8 << ") returned " << sum_id2 << ", expected " << expected_sum_id2 << "\n";
            return 1;
        }
        if (state.addition_miss_count() != 2) {
            std::cerr << "FAIL: addition miss count is " << state.addition_miss_count() << ", expected 2\n";
            return 1;
        }
        // repeat the addition to check that it hits the cache
        const std::uint16_t sum_id2_cached = state.add_ids(id7, id8);
        if (sum_id2_cached != expected_sum_id2) {
            std::cerr << "FAIL: add_ids(" << id7 << ", " << id8 << ") returned " << sum_id2_cached << " on second call, expected " << expected_sum_id2 << "\n";
            return 1;
        }
        if (state.addition_miss_count() != 2) {
            std::cerr << "FAIL: addition miss count is " << state.addition_miss_count() << " after second call, expected 2\n";
            return 1;
        }

        // validate that out-of-range addition IDs throw an exception
        try {
            state.add_ids(state.size(), 0);
            std::cerr << "FAIL: out-of-range add_ids did not throw an exception\n";
            return 1;
        } catch (const std::out_of_range&) {
            // expected
            // addition miss count should not have increased
            if (state.addition_miss_count() != 2) {
                std::cerr << "FAIL: addition miss count is " << state.addition_miss_count() << " after out-of-range add_ids, expected 2\n";
                return 1;
            }
        } catch (const std::exception& error) {
            std::cerr << "FAIL: out-of-range add_ids threw unexpected exception: " << error.what() << "\n";
            return 1;
        }

        // retry multiply IDs (2, 11) ensure that multiplication and addition caches are independent
        const std::uint16_t product_id_retry = state.multiply_ids(id1, id2);
        if (product_id_retry != expected_product_id) {
            std::cerr << "FAIL: multiply_ids(" << id1 << ", " << id2 << ") returned " << product_id_retry << " on retry, expected " << expected_product_id << "\n";
            return 1;
        }
        if (state.multiplication_miss_count() != 3) {
            std::cerr << "FAIL: multiplication miss count is " << state.multiplication_miss_count() << " after retry, expected 3\n";
            return 1;
        }

    } catch (const std::exception& error) {
        std::cerr << "FAIL: unexpected exception: " << error.what() << "\n";
        return 1;
    }
}