#include <algebra/finite_fields/detail/element_id.hpp>
#include <algebra/finite_fields/detail/original_finite_field.hpp>
#include <algebra/finite_fields/finite_fields.hpp>
#include <cstdint>
#include <exception>
#include <iostream>
namespace {

using Field = algebra::FiniteField<11, 3>;
using Element = Field::Element;
using InternalField = algebra::detail::original_fields::FiniteField<11, 3>;
}  // namespace

int main() {
    try {
        Field::initialize();
        InternalField::initialize();
        int failures = 0;

        // verify encode(decode(id)) == id for all valid ids (0 <= id < Field::cardinality())
        for (std::uint32_t id = 0; id < Field::cardinality(); ++id) {
            const Element element = algebra::detail::cached_fields::decode<Field>(id);
            const std::uint32_t encoded_id = algebra::detail::cached_fields::encode<Field>(element);
            if (encoded_id != id) {
                std::cerr << "FAIL: id " << id << " encoded to " << encoded_id << " after decoding\n";
                ++failures;
            }
        }

        // check known mappings for the first few elements of the field
        const Element zero = Element::zero();
        const Element one = Element::one();
        const Element x = Element::from_coefficients({0, 1, 0});                  // x
        const Element x2 = Element::from_coefficients({0, 0, 1});                 // x^2
        const Element TWOx2_THREEx_FOUR = Element::from_coefficients({4, 3, 2});  // 2x^2 + 3x + 4
        const Element eleven = Element::from_integer(11);                         // 0 in the field

        if (algebra::detail::cached_fields::encode<Field>(zero) != 0) {
            std::cerr << "FAIL: zero encoded to non-zero\n";
            ++failures;
        }
        if (algebra::detail::cached_fields::encode<Field>(one) != 1) {
            std::cerr << "FAIL: one encoded to non-one\n";
            ++failures;
        }
        if (algebra::detail::cached_fields::encode<Field>(x) != 11) {
            std::cerr << "FAIL: x encoded to non-11\n";
            ++failures;
        }
        if (algebra::detail::cached_fields::encode<Field>(x2) != 121) {
            std::cerr << "FAIL: x^2 encoded to non-121\n";
            ++failures;
        }
        if (algebra::detail::cached_fields::encode<Field>(TWOx2_THREEx_FOUR) != 4 + 3 * 11 + 2 * 121) {  // 279
            std::cerr << "FAIL: 2x^2 + 3x + 4 encoded to non-279\n";
            ++failures;
        }
        if (algebra::detail::cached_fields::encode<Field>(eleven) != 0) {
            std::cerr << "FAIL: 11 encoded to non-zero\n";
            ++failures;
        }

        // verify rejection of invalid IDs for decode
        for (std::uint32_t invalid_id : {1331, 1332, 65535}) {
            try {
                const Element element = algebra::detail::cached_fields::decode<Field>(invalid_id);
                std::cerr << "FAIL: decode accepted invalid id " << invalid_id << '\n';
                ++failures;
            } catch (const std::out_of_range&) {
                // expected
            } catch (const std::exception& error) {
                std::cerr << "FAIL: decode threw unexpected exception for id " << invalid_id
                          << ": " << error.what() << '\n';
                ++failures;
            }
        }

        // verify that public and internal field modulus are the same
        if (Field::modulus() != InternalField::modulus()) {
            std::cerr << "FAIL: modulus polynomials differ between public and internal field\n";
            ++failures;
        }

        // verify that public and internal field elements decode to the same value for all valid ids
        for (std::uint32_t id = 0; id < Field::cardinality(); ++id) {
            const Element reference = algebra::detail::cached_fields::decode<Field>(id);
            const InternalField::Element internal = algebra::detail::cached_fields::decode<InternalField>(id);
            if (reference.coefficients() != internal.coefficients()) {
                std::cerr << "FAIL: decode mismatch for id " << id << ": public field element " << reference
                          << ", internal field element " << internal << '\n';
                ++failures;
            }

            if (algebra::detail::cached_fields::encode<Field>(reference) != algebra::detail::cached_fields::encode<InternalField>(internal)) {
                std::cerr << "FAIL: encode mismatch for id " << id << ": public field element " << reference
                          << ", internal field element " << internal << '\n';
                ++failures;
            }
        }

        // verify reciprocal of 2 is 6 (since 2 * 6 = 12 ≡ 1 mod 11)
        const InternalField::Element two = InternalField::Element::from_integer(2);
        const InternalField::Element six = InternalField::Element::from_integer(6);
        if (two.reciprocal() != six) {
            std::cerr << "FAIL: reciprocal of 2 is not 6\n";
            ++failures;
        }

        // verify x*x*x has coefficients {7, 10, 0}
        const InternalField::Element x_internal = InternalField::Element::from_coefficients({0, 1, 0});
        const InternalField::Element x_cubed = x_internal.multiply(x_internal).multiply(x_internal);
        const InternalField::Element expected_x_cubed = InternalField::Element::from_coefficients({7, 10, 0});
        if (x_cubed != expected_x_cubed) {
            std::cerr << "FAIL: x^3 has coefficients " << x_cubed << ", expected " << expected_x_cubed << '\n';
            ++failures;
        }

        if (failures != 0) {
            std::cerr << failures << " check(s) failed\n";
            return 1;
        }
        std::cout << "All encode/decode tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Unexpected exception: " << error.what() << '\n';
        return 1;
    }
}
