#include <algebra/finite_fields/detail/cached_finite_field.hpp>
#include <algebra/matrices/matrices.hpp>

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>

// a lot more help from AI with these complex-ish / verbose tests

namespace {

using Field = algebra::FiniteField<11, 3>;
using Element = Field::Element;
using OriginalField = algebra::detail::original_fields::FiniteField<11, 3>;

void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template<class Actual, class Expected>
void check_same(const Actual& actual, const Expected& expected, const char* message) {
    if (actual.coefficients() != expected.coefficients()) {
        std::ostringstream details;
        details << message << ": expected " << expected << ", got " << actual;
        throw std::runtime_error(details.str());
    }
}

template<class Exception, class Operation>
void expect_exception(Operation operation, const char* message) {
    try {
        operation();
    } catch (const Exception&) {
        return;
    }
    throw std::runtime_error(message);
}

template<class Operation, class MissCount>
void check_cached_call(Operation operation, MissCount miss_count, const Element& expected) {
    const auto before = miss_count();
    check(operation() == expected, "incorrect result on a cache miss");
    check(miss_count() == before + 1, "first wrapper call did not record one miss");
    check(operation() == expected, "incorrect result on a cache hit");
    check(miss_count() == before + 1, "repeated wrapper call recorded another miss");
}

void check_cache_delegation() {
    auto& state = algebra::detail::cached_fields::state_for<OriginalField>();
    const Element two = Element::from_integer(2);
    const Element x = Element::from_index(11);
    check(state.addition_miss_count() == 0 && state.multiplication_miss_count() == 0 &&
              state.negation_miss_count() == 0 && state.reciprocal_miss_count() == 0,
          "factories unexpectedly filled arithmetic caches");

    check_cached_call([&] { return two.add(x); }, [&] { return state.addition_miss_count(); },
                      Element::from_coefficients({2, 1, 0}));
    check_cached_call([&] { return two.multiply(x); }, [&] { return state.multiplication_miss_count(); },
                      Element::from_coefficients({0, 2, 0}));
    check_cached_call([&] { return x.negate(); }, [&] { return state.negation_miss_count(); },
                      Element::from_coefficients({0, 10, 0}));
    check_cached_call([&] { return two.reciprocal(); }, [&] { return state.reciprocal_miss_count(); },
                      Element::from_integer(6));
    check_cached_call([&] { return two.add(Element::from_integer(9)); },
                      [&] { return state.addition_miss_count(); }, Element::zero());

    const auto before = state.reciprocal_miss_count();
    for (int attempt = 0; attempt < 2; ++attempt) {
        expect_exception<std::domain_error>([] { Element::zero().reciprocal(); },
                                            "zero reciprocal did not throw");
        check(state.reciprocal_miss_count() == before, "zero reciprocal changed the miss count");
    }
}

// Run the same API checks for both the cached field and an uncached field size
template<class TestField, class ReferenceField>
void check_api_equivalence() {
    using TestElement = typename TestField::Element;
    using ReferenceElement = typename ReferenceField::Element;
    using Polynomial = typename TestElement::polynomial_type;

    static_assert(std::is_same_v<typename TestElement::parent_type, TestField>);
    static_assert(std::is_same_v<typename TestElement::algebra, algebra::FieldDescriptor>);
    static_assert(std::is_same_v<decltype(std::declval<const TestElement&>().representative()), const Polynomial&>);
    static_assert(std::is_same_v<decltype(std::declval<const TestElement&>().parent()), const TestField&>);
    static_assert(std::is_same_v<decltype(std::declval<TestElement>() * std::declval<TestElement>()), TestElement>);

    const TestField field;
    const std::uint64_t count = TestField::cardinality();
    check(count == ReferenceField::cardinality(), "field cardinality differs");
    check(TestField::characteristic == ReferenceField::characteristic, "characteristic differs");
    check(TestField::extension_degree == ReferenceField::extension_degree, "extension degree differs");
    check(TestField::modulus() == ReferenceField::modulus(), "field modulus differs");
    check(TestElement{}.is_zero() && TestField::zero().is_zero(), "default/zero factory is not zero");
    check(TestField::one().is_one(), "one factory is not one");
    check_same(TestField::alpha(), ReferenceField::alpha(), "alpha factory");
    check_same(TestField::element(-1), ReferenceField::element(-1), "integer field factory");
    check_same(TestField::element({4, 3, 2}), ReferenceField::element({4, 3, 2}), "coefficient field factory");
    check_same(TestElement::from_coefficients({-1, 12, 22}),
               ReferenceElement::from_coefficients({-1, 12, 22}), "coefficient normalization");

    const Polynomial x = Polynomial::variable();
    const Polynomial cubic = x.multiply(x).multiply(x);
    check_same(TestElement::from_polynomial(cubic), ReferenceElement::from_polynomial(cubic),
               "polynomial reduction");

    // Every enumerated value and every unary result must match original arithmetic
    for (std::uint64_t id = 0; id < count; ++id) {
        const TestElement actual = TestElement::from_index(id);
        const ReferenceElement expected = ReferenceElement::from_index(id);
        check_same(actual, expected, "enumeration");
        check(actual.representative() == expected.representative(), "polynomial representative differs");
        check(actual.is_zero() == expected.is_zero(), "zero predicate differs");
        check(actual.is_one() == expected.is_one(), "one predicate differs");
        check_same(actual.negate(), expected.negate(), "negation");
        check_same(-actual, expected.negate(), "unary minus operator");
        if (id != 0) {
            check_same(actual.reciprocal(), expected.reciprocal(), "reciprocal");
            check(actual.multiply(actual.reciprocal()).is_one(), "inverse identity failed");
        }
        for (std::size_t degree = 0; degree < TestField::extension_degree; ++degree) {
            check(actual.coefficient(degree) == expected.coefficient(degree), "coefficient lookup differs");
        }
    }

    // Boundary and extension-field values exercise binary methods and inherited operators
    const std::array<std::uint64_t, 9> ids{
        0, 1, 2, TestField::characteristic - 1, TestField::characteristic,
        TestField::characteristic + 1, count / 3, count / 2, count - 1};
    for (const auto left : ids) {
        const TestElement a = TestElement::from_index(left);
        const ReferenceElement original_a = ReferenceElement::from_index(left);
        for (const auto right : ids) {
            const TestElement b = TestElement::from_index(right);
            const ReferenceElement original_b = ReferenceElement::from_index(right);
            check((a == b) == (original_a == original_b), "equality differs");
            check_same(a + b, original_a.add(original_b), "addition operator");
            check_same(a - b, original_a.add(original_b.negate()), "subtraction operator");
            check_same(a * b, original_a.multiply(original_b), "multiplication operator");

            TestElement compound = a;
            compound += b;
            check_same(compound, original_a.add(original_b), "addition assignment");
            compound = a;
            compound -= b;
            check_same(compound, original_a.add(original_b.negate()), "subtraction assignment");
            compound = a;
            compound *= b;
            check_same(compound, original_a.multiply(original_b), "multiplication assignment");
            if (right != 0) {
                const auto quotient = original_a.multiply(original_b.reciprocal());
                check_same(a / b, quotient, "division operator");
                compound = a;
                compound /= b;
                check_same(compound, quotient, "division assignment");
            }
        }
    }

    using TestMatrix = algebra::Matrix<TestElement, 2, 2>;
    using ReferenceMatrix = algebra::Matrix<ReferenceElement, 2, 2>;
    const TestMatrix matrix{{TestField::one(), TestField::alpha()},
                            {TestField::element(2), TestElement::from_index(count - 1)}};
    const ReferenceMatrix original_matrix{{ReferenceField::one(), ReferenceField::alpha()},
                                          {ReferenceField::element(2), ReferenceElement::from_index(count - 1)}};
    const TestMatrix product = matrix.multiply(matrix);
    const ReferenceMatrix original_product = original_matrix.multiply(original_matrix);
    for (std::size_t row = 0; row < 2; ++row) {
        for (std::size_t column = 0; column < 2; ++column) {
            check_same(product(row, column), original_product(row, column), "matrix product entry");
        }
    }

    const TestElement two = TestElement::from_integer(2);
    check_same(field.combine(algebra::Addition{}, two, two), ReferenceField::element(4),
               "field structure addition");
    check_same(field.reciprocal(two), ReferenceField::element(2).reciprocal(),
               "field structure reciprocal");
    check(&two.parent() == &TestElement::one().parent(), "elements have different parent instances");
    check(two.parent().cardinality() == count, "parent metadata differs");

    std::ostringstream actual_output;
    std::ostringstream expected_output;
    actual_output << TestElement::from_index(count - 1);
    expected_output << ReferenceElement::from_index(count - 1);
    check(actual_output.str() == expected_output.str(), "stream output differs");

    for (const std::uint64_t invalid : {count, count + 1, std::uint64_t{65536},
                                        std::numeric_limits<std::uint64_t>::max()}) {
        expect_exception<std::out_of_range>([&] { TestElement::from_index(invalid); },
                                            "invalid enumeration index was accepted");
    }
    expect_exception<std::out_of_range>([] { TestElement::one().coefficient(TestField::extension_degree); },
                                        "invalid coefficient index was accepted");
    expect_exception<std::domain_error>([] { TestElement::one() / TestElement::zero(); },
                                        "division by zero was accepted");
}

} // namespace

int main() {
    try {
        static_assert(sizeof(Element) == sizeof(std::uint16_t), "cached elements must remain compact");
        check_cache_delegation();
        check_api_equivalence<Field, OriginalField>();
        check(Element::from_integer(11).is_zero(), "integer 11 must embed as zero");
        check(Element::from_index(11) == Field::alpha(), "index 11 must represent x");

        using FallbackField = algebra::FiniteField<5, 2>;
        using FallbackOriginal = algebra::detail::original_fields::FiniteField<5, 2>;
        check_api_equivalence<FallbackField, FallbackOriginal>();

        // from_index must validate the modulus even before any arithmetic/view call
        using InvalidField = algebra::FiniteField<11, 3, algebra::PolynomialModulus<1>>;
        expect_exception<std::invalid_argument>([] { InvalidField::Element::from_index(0); },
                                               "enumeration skipped modulus validation");

        std::cout << "All cached element API and fallback tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
