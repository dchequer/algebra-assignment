#ifndef ALGEBRA_FINITE_FIELDS_DETAIL_CACHED_FINITE_FIELD_HPP
#define ALGEBRA_FINITE_FIELDS_DETAIL_CACHED_FINITE_FIELD_HPP

#include <algebra/finite_fields/detail/cache_state.hpp>
#include <algebra/finite_fields/detail/original_finite_field.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace algebra {

template <std::uint32_t P, std::size_t K = 1, class ModulusPolicy = FirstEnumeratedIrreducible>
class FiniteField;

template <std::uint32_t P, std::size_t K, class ModulusPolicy>
class FiniteFieldElement
    : public AdditiveOperators<FiniteFieldElement<P, K, ModulusPolicy>>,
      public MultiplicativeOperators<FiniteFieldElement<P, K, ModulusPolicy>>,
      public DivisionOperators<FiniteFieldElement<P, K, ModulusPolicy>> {
    static_assert(is_prime(P));
    static_assert(K > 0);

   private:
    using OriginalField = ::algebra::detail::original_fields::FiniteField<P, K, ModulusPolicy>;
    using OriginalElement = typename OriginalField::Element;

    static constexpr bool cache_enabled = (P == 11 && K == 3);

    // if cache_enabled, the element is stored as an ID (uint16_t) and arithmetic is delegated to the cache state
    // if !cache_enabled, the element is stored as an OriginalElement and arithmetic is delegated to the original field implementation
    // this allows the cached field to be used as a drop-in replacement
    using Storage =
        std::conditional_t<cache_enabled, std::uint16_t, OriginalElement>;

    Storage value_{};

    struct KnownValue {};

    FiniteFieldElement(Storage value, KnownValue) : value_(std::move(value)) {}

    static FiniteFieldElement from_known_id(std::uint16_t id) {
        return FiniteFieldElement(id, KnownValue{});
    }
    static FiniteFieldElement wrap_original(OriginalElement value) {
        if constexpr (cache_enabled) {
            return from_known_id(::algebra::detail::cached_fields::encode<OriginalField>(value));
        } else {
            return FiniteFieldElement(std::move(value), KnownValue{});
        }
    }

    const OriginalElement& original_value() const {
        if constexpr (cache_enabled) {
            return ::algebra::detail::cached_fields::state_for<OriginalField>().value(value_);
        } else {
            return value_;
        }
    }

   public:
    using parent_type = FiniteField<P, K, ModulusPolicy>;
    using algebra = ::algebra::FieldDescriptor;
    using coefficient_type = typename OriginalElement::coefficient_type;
    using polynomial_type = typename OriginalElement::polynomial_type;

    FiniteFieldElement() {
        OriginalField::initialize();

        if constexpr (cache_enabled) {
            value_ = 0;  // zero element ID
        } else {
            value_ = OriginalElement::zero();
        }
    }

    static FiniteFieldElement from_index(std::uint64_t index) {
        if constexpr (cache_enabled) {
            if (index >= OriginalField::cardinality()) {
                throw std::out_of_range("Finite field element index is out of range");
            }
            OriginalField::initialize();
            return from_known_id(static_cast<std::uint16_t>(index));
        } else {
            return wrap_original(OriginalElement::from_index(index));
        }
    }
    static FiniteFieldElement from_integer(std::int64_t value) {
        OriginalElement original = OriginalElement::from_integer(value);
        return wrap_original(original);
    }
    static FiniteFieldElement from_coefficients(std::initializer_list<std::int64_t> values) {
        OriginalElement original = OriginalElement::from_coefficients(values);
        return wrap_original(original);
    }
    static FiniteFieldElement from_polynomial(const polynomial_type& value) {
        OriginalElement original = OriginalElement::from_polynomial(value);
        return wrap_original(original);
    }

    static FiniteFieldElement zero() {
        return FiniteFieldElement{};
    }
    static FiniteFieldElement one() {
        return from_integer(1);
    }

    FiniteFieldElement add(const FiniteFieldElement& other) const {
        if constexpr (cache_enabled) {
            std::uint16_t result_id = ::algebra::detail::cached_fields::state_for<OriginalField>().add_ids(value_, other.value_);
            return from_known_id(result_id);
        } else {
            OriginalElement sum = original_value().add(other.original_value());
            return wrap_original(sum);
        }
    }
    FiniteFieldElement multiply(const FiniteFieldElement& other) const {
        if constexpr (cache_enabled) {
            std::uint16_t result_id = ::algebra::detail::cached_fields::state_for<OriginalField>().multiply_ids(value_, other.value_);
            return from_known_id(result_id);
        } else {
            OriginalElement product = original_value().multiply(other.original_value());
            return wrap_original(product);
        }
    }
    FiniteFieldElement negate() const {
        if constexpr (cache_enabled) {
            std::uint16_t result_id = ::algebra::detail::cached_fields::state_for<OriginalField>().negate_id(value_);
            return from_known_id(result_id);
        } else {
            OriginalElement negated = original_value().negate();
            return wrap_original(negated);
        }
    }
    FiniteFieldElement reciprocal() const {
        if constexpr (cache_enabled) {
            std::uint16_t result_id = ::algebra::detail::cached_fields::state_for<OriginalField>().reciprocal_id(value_);
            return from_known_id(result_id);
        } else {
            OriginalElement reciprocal = original_value().reciprocal();
            return wrap_original(reciprocal);
        }
    }

    bool is_zero() const {
        if constexpr (cache_enabled) {
            return value_ == 0;
        } else {
            return original_value().is_zero();
        }
    }
    bool is_one() const {
        if constexpr (cache_enabled) {
            return value_ == 1;
        } else {
            return original_value().is_one();
        }
    }
    bool operator==(const FiniteFieldElement& other) const {
        if constexpr (cache_enabled) {
            return value_ == other.value_;
        } else {
            return original_value() == other.original_value();
        }
    }

    coefficient_type coefficient(std::size_t index) const {
        return original_value().coefficient(index);
    }
    std::array<coefficient_type, K> coefficients() const {
        return original_value().coefficients();
    }
    const polynomial_type& representative() const {
        return original_value().representative();
    }
    const parent_type& parent() const {
        static const parent_type instance;
        return instance;
    }

    friend std::ostream& operator<<(
        std::ostream& out,
        const FiniteFieldElement& element) {
        return out << element.original_value();
    }
};

template <std::uint32_t P, std::size_t K, class ModulusPolicy>
class FiniteField
    : public FieldStructure<FiniteFieldElement<P, K, ModulusPolicy>> {
   private:
    using OriginalField =
        ::algebra::detail::original_fields::FiniteField<P, K, ModulusPolicy>;

   public:
    using Element = FiniteFieldElement<P, K, ModulusPolicy>;
    using Base = typename OriginalField::Base;
    using PolynomialType = typename Element::polynomial_type;

    static constexpr std::uint32_t characteristic = P;
    static constexpr std::size_t extension_degree = K;

    FiniteField() {
        initialize();
    }

    static void initialize() {
        OriginalField::initialize();
    }
    static const PolynomialType& modulus() {
        return OriginalField::modulus();
    }
    static std::uint64_t cardinality() {
        return OriginalField::cardinality();
    }

    static Element element(std::int64_t value) {
        return Element::from_integer(value);
    }
    static Element element(std::initializer_list<std::int64_t> values) {
        return Element::from_coefficients(values);
    }

    static Element zero() {
        return Element::zero();
    }
    static Element one() {
        return Element::one();
    }
    static Element alpha() {
        return Element::from_polynomial(PolynomialType::variable());
    }
};

}  // namespace algebra
#endif  // ALGEBRA_FINITE_FIELDS_DETAIL_CACHED_FINITE_FIELD_HPP
