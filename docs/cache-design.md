# Cached finite field design and implementation checklist

This is the accepted design for the cached provider: compact element IDs,
shared original-value catalogs, and lazily filled dense arithmetic tables.
Cache misses reuse the original arithmetic and its deterministic modulus.
Existing matrix and determinant callers retain their public interface.

This file records the detailed work behind cards 05–07 in
[the Kanban](../KANBAN.md). The architecture is agreed; implementation and
validation tasks below remain open unless explicitly checked.

## Current increment and file placement

The ID-conversion increment is complete against the existing original field.
The [conversion test](../tests/encode_decode_tests.tpp) covers all 1331 valid-ID
round trips, six known mappings, and three invalid IDs with the expected
exception type; the user reports all checks passing.

The original field header is now retained under an internal namespace, with
its mathematical method bodies unchanged. The public/internal modulus,
coefficient-array, and encoding comparisons pass for all 1331 IDs.

The private-original arithmetic checks now construct internal elements and pass,
including the reciprocal of 2 and the reduced cubic product. The registered
CTest targets pass.

The immutable original-value catalog and shared accessor are implemented in
[cache_state.hpp](../dependencies/algebra-finite-fields-cached/include/algebra/finite_fields/detail/cache_state.hpp).
The [catalog tests](../tests/cache_state_tests.tpp) pass size, all 1331 mappings,
shared-object identity, and bounds checks. All three registered tests pass.

The multiplication ID cache is implemented. The user reports all tests pass;
the state test covers misses, repeated hits, cached zero, cubic reduction, and
invalid-left-ID rejection. The shared accessor now returns a non-const reference
to a non-const state. Additional multiplication checks remain listed in
[section 7](#7-implement-multiplication-hits-and-misses).

The separate addition ID cache is also implemented. The user reports all tests
pass; checks cover hits, coefficient wraparound, cached zero, invalid-left-ID
rejection without increasing the addition miss count, and table independence.
Right-operand bounds checks remain to be exercised for both binary operations.

The unary negation and reciprocal ID caches are implemented and exhaustively
checked against original arithmetic. A fresh local state exercises all 1331
negations and 1330 nonzero reciprocals on both miss and hit paths. Two attempts
to invert zero throw without recording a successful miss; unary work leaves
binary miss counters at zero. All three registered CTest targets pass after
rebuilding the expanded tests.

The public field and element wrappers are now implemented in
[cached_finite_field.hpp](../dependencies/algebra-finite-fields-cached/include/algebra/finite_fields/detail/cached_finite_field.hpp).
The [wrapper tests](../tests/cached_element_tests.tpp) check compact storage,
construction, enumeration and unary equivalence, selected binary operands,
cache reuse, operators, views, output, field interfaces, exceptions, and the
original-storage fallback for `<5, 2>`. The same seven determinant fixtures pass
with original and cached elements in separate executables. All five registered
CTest targets pass, with no reported diagnostics in the edited code.

Public entry headers and CMake provider selection are complete. The cache
argument `ALGEBRA_ASSIGNMENT_BACKEND` chooses `original` (default) or `cached`.
Both Release configurations pass all five tests and match the seeded dimension-8
reference. The [quick backend comparison](../results/backend-check/README.md)
contains 18 fresh-process timings at dimensions 64, 128, and 256, with verified
coefficients and determinant identities. The current build selects cached,
Release, dimension 256. Comparison figures and cached profiling remain for the
measurement/report stage.

The functions belong in the cached dependency's internal
[element_id.hpp](../dependencies/algebra-finite-fields-cached/include/algebra/finite_fields/detail/element_id.hpp).
That header contains the implementation alongside its pseudocode comments.

Planned ownership:

```text
dependencies/algebra-finite-fields-cached/
    CMakeLists.txt                         # implemented replacement provider
    include/algebra/finite_fields/
        finite_fields.hpp                  # public path stays the same
        finite_field.hpp                   # public cached types
        detail/
            cached_finite_field.hpp        # implemented public types, staged for testing
            element_id.hpp                 # encode/decode helpers
            original_finite_field.hpp      # privately named original classes
            cache_state.hpp                # catalog and result tables

tests/
    encode_decode_tests.tpp                # current conversion tests
    determinant_tests.tpp                  # existing matrix reference cases
    cached_element_tests.tpp               # public wrapper and fallback checks
```

The helper, internal original-field header, shared catalog, all four lazy
arithmetic tables, public classes, provider target, and public entry headers are
implemented. Conversion, state, and wrapper tests have CMake/CTest targets, and
the determinant fixtures run against both implementations. Selected matrix
products are also compared entry by entry with private original elements.

Use `algebra::detail::cached_fields` for the helper namespace. Both helpers can
take `OriginalField` as a template parameter, so their bodies work with the
current original field and the privately named original field later. Derive
the base and coefficient count from that field's metadata; they are 11 and 3
for the accepted fast path. Do not make this helper include the future public
cached field header: the cached field will itself need the helper.

The conversion test target uses both the current original field dependency
and the new cached dependency's include directory, with C++20 enabled.
The normal application continues to use its current provider until the full
replacement target is ready.

## 1 Retain the original arithmetic

- [ ] Prepare a separate copy/fork of the finite-field dependency at the pinned
  original revision in [the dependency map](./dependency-map.md).
- [x] Keep the original field class definitions under an internal namespace,
  preserving their mathematical method bodies.
- [x] Leave standard-library includes and shared polynomial/prime-field types
  in their normal namespaces; relocate only the relevant field definitions.
- [ ] Expose the original types internally as `OriginalField` and
  `OriginalElement` for conversion, cache misses, and reference tests.
- [x] Verify the internal modulus, element coefficients, and IDs against the
  public original field for every valid ID.
- [x] Run the known reciprocal and cubic-product checks on internal elements.

```text
inside the replacement dependency:
    OriginalField = privately named original field implementation
    OriginalElement = OriginalField.Element

    PublicField = existing public field interface
    PublicField.Element = CachedElement
```

Original calls must resolve to the original implementation rather than
recursively reentering the cached wrapper.

Next concrete steps:

1. Create the planned `detail/original_finite_field.hpp` under the cached
   dependency's existing `include/algebra/finite_fields` directory.
2. Copy the pinned upstream `finite_field.hpp` into it. Keep the original
   includes outside any namespace and keep the arithmetic method bodies intact.
3. Change the field definitions' enclosing namespace from `algebra` to
   `algebra::detail::original_fields`. Both the field and element class
   definitions must move together so their parent relationship stays internal.
4. In the conversion test, include the new internal header alongside the
   original public field header. The existing test target already has both
   required include roots.
5. Keep the public original field as a reference and exercise the same helpers
   with the internal field type. Compare modulus and coefficients, since the
   reference and internal element classes are different C++ types.

```text
ReferenceField = current public original FiniteField<11, 3>
InternalField = privately named original FiniteField<11, 3>

initialize both
check their selected moduli agree

for id from 0 through 1330:
    reference = ReferenceField.Element.from_index(id)
    internal = decode<InternalField>(id)

    check their coefficient arrays agree
    check encode<InternalField>(internal) == id

check the internal field still produces the known arithmetic results
```

A lasting original implementation needs its own definitions under internal
names. In a build selecting the cached provider, an alias to the public
`algebra::FiniteField` would refer to the cached type; it would not retain the
original implementation. Keep the public provider selection unchanged during
this preparation step.

## 2 Encode and decode field values

- [x] Create the internal helper header scaffold.
- [x] Define `encode<OriginalField>(original_element)` in that header.
- [x] Define `decode<OriginalField>(id)` in that header.
- [x] Test all 1331 valid IDs against the original implementation before adding caches.
- [x] Add the individual known-value checks listed below.
- [x] Change the decode guard to the original index constructor's
  `std::out_of_range` exception type.
- [x] Test invalid IDs, confirming both rejection and the expected exception type.

For coefficients in constant-first order:

```text
value = a0 + a1*x + a2*x²
id = a0 + 11*a1 + 121*a2
```

| Value | Coefficients | ID |
| --- | --- | ---: |
| Zero | `[0, 0, 0]` | 0 |
| One | `[1, 0, 0]` | 1 |
| Two | `[2, 0, 0]` | 2 |
| `x` | `[0, 1, 0]` | 11 |
| `2*x` | `[0, 2, 0]` | 22 |
| `4 + 3*x + 2*x²` | `[4, 3, 2]` | 279 |

```text
function encode<OriginalField>(original_element):
    id = 0
    place_value = 1

    for degree from 0 up to OriginalField.extension_degree, excluding the end:
        coefficient = original_element.coefficient(degree)
        digit = coefficient.representative()
        id = id + digit * place_value
        place_value = place_value * OriginalField.characteristic

    return id

function decode<OriginalField>(id):
    require 0 <= id < OriginalField.cardinality()
    return OriginalField.Element.from_index(id)
```

Decode with `from_index`, not `from_integer`: label 11 represents `x`, whereas
the integer 11 embeds as zero. Read digits from the normalized original element.

The first test target should check the following using ordinary original elements:

```text
initialize OriginalField

for id from 0 through 1330:
    value = decode<OriginalField>(id)
    check encode<OriginalField>(value) == id

check zero maps to 0
check one maps to 1
check x maps to 11
check x² maps to 121
check coefficients [4, 3, 2] map to 279
check integer 11 maps to 0
check invalid decode IDs preserve the expected bounds errors
```

The [conversion test](../tests/encode_decode_tests.tpp) now includes the known
values and invalid IDs `Field.cardinality()`, `Field.cardinality() + 1`, and
`65535`. It catches the expected `out_of_range` inside each invalid-input test,
so the outer unexpected-exception handler does not mistake an expected
rejection for a failed test.

```text
for invalid_id in [1331, 1332, 65535]:
    try:
        decode<Field>(invalid_id)
        record a failure because no exception was thrown
    catch out_of_range:
        this rejection test passed
    catch another standard exception:
        record a failure because the exception type was wrong
```

## 3 Store compact IDs in cached elements

- [x] Implement a cached element whose stored value is an ID.
- [x] Preserve initialization and field validation for default construction.
- [x] Implement equality, zero, and one checks using IDs.
- [x] Restrict the 16-bit optimized representation to the agreed small field;
  delegate other template instantiations to the original representation.

```text
CachedElement:
    private id

equals(other): return this.id == other.id
is_zero():    return this.id == 0
is_one():     return this.id == 1
```

For the `FiniteField<11, 3>` fast path, IDs `0..1330` fit in 16 bits. This reduces
per-matrix-entry storage independently of the savings from cached arithmetic.
The [wrapper walkthrough](#9-preserve-construction-and-public-compatibility)
explains how to connect this representation to the existing API without storing
another original element inside every cached matrix entry.

## 4 Initialize shared state once per field type

The catalog increment is complete in
[cache_state.hpp](../dependencies/algebra-finite-fields-cached/include/algebra/finite_fields/detail/cache_state.hpp),
templated on the original field type. The internal original implementation
supplies every catalog value. Multiplication, addition, negation, and reciprocal
result tables are all implemented.

- [x] Construct a catalog of exactly `OriginalField.cardinality()` original elements.
- [x] Expose read-only size and value access; keep references stable after construction.
- [x] Provide a once-initialized shared accessor for each original field type.
- [x] Verify catalog size, all ID-to-value mappings, repeated-accessor identity,
  and rejection of an out-of-range catalog index.
- [x] Build an immutable catalog of original values indexed by ID.
- [x] Permit arithmetic result-table updates while retaining read-only catalog
  access through a writable shared state.
- [x] Allocate the arithmetic result tables and fill them with the empty marker.
- [x] Keep catalog storage stable after initialization.
- [x] Keep states separate by characteristic, extension degree, and modulus policy.
- [x] Ensure initialization uses original elements and integers, avoiding recursive
  initialization through cached elements.

```text
FieldState:
    original_values[1331]
    addition_results[1331 * 1331]
    multiplication_results[1331 * 1331]
    negation_results[1331]
    reciprocal_results[1331]

function initialize_field_state_once():
    OriginalField.initialize()
    reserve final catalog capacity

    for id from 0 through 1330:
        append OriginalElement.from_index(id) to original_values

    allocate arithmetic result tables
    fill each result entry with EMPTY
```

Catalog entries must be `OriginalElement` objects. Every element and matrix of
the same field type shares this state. Future MPI processes each have their own
local state while agreeing on the field and encoding.

First catalog-only increment:

```text
class CacheState<OriginalField>:
    OriginalElement = OriginalField.Element
    private original_values: sequence of OriginalElement

    constructor:
        OriginalField.initialize()
        count = OriginalField.cardinality()
        reserve original_values capacity for count entries

        for id from 0 up to count, excluding the end:
            append decode<OriginalField>(id)

    size():
        return original_values.size()

    value(id):
        validate id against original_values.size()
        return const reference to original_values[id]

function state_for<OriginalField>():
    construct one static CacheState<OriginalField> on first access
    return reference to that state
```

The catalog maps one ID to its original value. Later result tables map operand
IDs to a result ID; they serve a different purpose. Keep the catalog unchanged
after construction so the cached element's eventual polynomial reference API
can safely refer into it. Initialization must use original elements exclusively,
so it does not recursively request its own unfinished cached state.

## 5 Use lazily filled dense tables

- [ ] Store result IDs in two-byte entries for this fast path.
- [ ] Reserve `65535` as `EMPTY`; zero remains a valid cached result.
- [ ] Calculate results only on misses rather than eagerly calculating every pair.
- [ ] Measure table initialization and total resident memory when benchmarking.

| Design | Advantage | Cost |
| --- | --- | --- |
| `unordered_map` | Stores requested combinations | Hashing, buckets, and node allocations |
| Dense table, selected | Direct indexing and predictable result storage | Allocates space for unused combinations |

There are `1331² = 1,771,561` ordered pairs. Each binary table has approximately
3.38 MiB of result storage; addition and multiplication together have about
6.76 MiB. Catalogs, unary tables, matrices, and allocation overhead are additional.
The smallest cached runs can consequently use more memory than the original.

## 6 Encode operand pairs

- [ ] Calculate pair keys using an integer wider than 16 bits.
- [ ] Use separate result tables for distinct operations.
- [ ] Retain ordered operand pairs in the first implementation.

```text
pair_key(left_id, right_id):
    return wide_integer(left_id) * 1331 + right_id
```

The maximum key is 1,771,560. For the request `2 * x`, the operand IDs are 2 and
11, giving key 2673. Separate addition and multiplication tables prevent their
different results from colliding at the same pair key. Commutativity-based
compression is a later optional optimization.

## 7 Implement multiplication hits and misses

- [x] Return a non-const reference to a non-const static state from `state_for`;
  keep `size()` and `value()` const and keep catalog entries private.
- [x] Allocate a multiplication table with `Q * Q` 16-bit entries, initially `EMPTY`.
- [x] Implement `multiply_ids(left_id, right_id)` with bounds checks before indexing.
- [x] Return a cached result ID directly on a hit.
- [x] Call original multiplication on a miss, encode its result, then store it.
- [x] Write the cache entry only after calculation and encoding succeed.
- [x] Expose a read-only multiplication miss count to verify reuse in tests.
- [x] Test a miss, a hit, a cached zero, polynomial reduction, and an invalid left ID.
- [ ] Test identity, reversed operands, an invalid right ID, and unchanged miss
  counts after rejected requests.
- [x] Correct the cubic-product comments to consistently describe `-x - 4`.

Implementation review: `result_id_` is currently a member used as scratch storage
by multiplication. Prefer a local result variable inside each operation; the
tables retain results between calls, while scratch variables need not be shared.

Implement this operation in
[cache_state.hpp](../dependencies/algebra-finite-fields-cached/include/algebra/finite_fields/detail/cache_state.hpp)
before introducing the public wrapper. The state accepts IDs and returns an ID;
on a miss it uses the existing original-value catalog to perform the calculation.
This table layout is for the selected 1331-element fast path.

```text
additional private state:
    ResultId = unsigned 16-bit integer
    EMPTY = maximum ResultId value                 // 65535, never a valid ID here
    multiplication_results: sequence of ResultId
    multiplication_misses = 0

constructor, after constructing the original-value catalog:
    Q = size(), represented as a wide index type
    allocate Q * Q result entries and fill them with EMPTY
    // Do not calculate any products yet.

function multiply_ids(left_id, right_id):
    Q = size()
    if either ID is outside [0, Q):
        throw out_of_range

    key = wide_index(left_id) * Q + right_id
    if multiplication_results[key] != EMPTY:
        return multiplication_results[key]

    original_result = value(left_id).multiply(value(right_id))
    result_id = encode<OriginalField>(original_result)
    multiplication_results[key] = ResultId(result_id)
    multiplication_misses += 1
    return result_id

function multiplication_miss_count() const:
    return multiplication_misses

function state_for<OriginalField>():
    construct one non-const static CacheState<OriginalField> on first access
    return non-const reference to that state
```

Use a wide type such as `std::size_t` **before** multiplying to form a key;
16-bit IDs do not imply 16-bit indexes. Obtain the empty marker from the 16-bit
type's maximum value (C++ provides `std::numeric_limits` via `<limits>`).
Do not use zero as the empty marker: zero is a valid product. Existing tests may
still bind the shared state to a const reference for their read-only checks.

Extend [cache_state_tests.tpp](../tests/cache_state_tests.tpp) using a fresh local
`State` for the arithmetic checks so its table starts empty. Keep the shared-state
identity checks as well. Suggested sequence:

| Request | Expected result ID | Expected miss-count behavior |
| --- | --- | --- |
| `2 * x`, IDs `(2, 11)` | `22` | Increase by one |
| Repeat `(2, 11)` | `22` | No change |
| `0 * x`, IDs `(0, 11)`, twice | `0` both times | Increase on first request only |
| `1 * x`, IDs `(1, 11)` | `11` | Increase by one |
| `x² * x`, IDs `(121, 11)` | `117`, encoding `[7, 10, 0]` | Increase by one |
| `x * 2`, IDs `(11, 2)` | `22` | Increase by one: a separate ordered key |
| `(Q, 0)` and `(0, Q)` | Throw `out_of_range` | No change |

The reduced cubic is `x³ = -x - 4 = 10x + 7` for the existing modulus.
A repeated correct result alone does not demonstrate a cache hit; the unchanged
miss count supplies that evidence. The counter changes only after a successful
miss calculation and store. This sequential design needs no thread synchronization
at this stage; each future MPI process has its own state.

Once the ID-level operation passes, the eventual public wrapper can delegate:

```text
function CachedElement.multiply(other):
    state = shared state for this field
    result_id = state.multiply_ids(this.id, other.id)
    return cached_element_from_known_id(result_id)
```

`cached_element_from_known_id` is an internal constructor/helper for already-valid
IDs. It must not recreate a polynomial or use integer embedding. The first
`2 * x` request computes and stores ID 22 at key 2673; the next returns ID 22.

## 8 Add the other cached operations

- [x] Add a separate binary addition cache.
- [x] Test addition misses, hits, coefficient wraparound, cached zero, invalid
  left IDs, and independence from multiplication.
- [ ] Test invalid right IDs for addition without changing its miss count.
- [x] Add unary negation and reciprocal caches.
- [x] Preserve the original zero-reciprocal exception without storing a result.
- [x] Test known unary results, unary bounds, and an unchanged reciprocal miss
  count after a zero-reciprocal exception.
- [x] Test repeated unary requests and repeat zero's reciprocal exception.
- [x] Compare all 1331 negations and 1330 nonzero reciprocals with the original.
- [x] Keep subtraction and division expressed through existing operator adapters.

Addition is implemented inside
[cache_state.hpp](../dependencies/algebra-finite-fields-cached/include/algebra/finite_fields/detail/cache_state.hpp).
Use a **separate** `Q * Q` table of 16-bit result IDs, filled with `EMPTY` in the
constructor, and a separate miss counter initialized to zero. The selected field
needs another approximately 3.38 MiB of result storage.

```text
function add_ids(left_id, right_id):
    Q = size()
    if either ID is outside [0, Q):
        throw out_of_range

    key = wide_index(left_id) * Q + right_id
    result_id = addition_results[key]             // local variable
    if result_id != EMPTY:
        return result_id

    original_result = value(left_id).add(value(right_id))
    result_id = encode<OriginalField>(original_result)
    addition_results[key] = result_id
    addition_misses += 1
    return result_id

function addition_miss_count() const:
    return addition_misses
```

Do not add the numerical IDs to obtain a result. IDs encode coefficient digits;
addition reduces each coefficient independently modulo 11, without carrying
between digits. For example, IDs `10` and `1` add to ID `0`, rather than ID `11`.

Extend the existing state tests with:

| Request | Expected result ID | Expected behavior |
| --- | --- | --- |
| `2 + x`, IDs `(2, 11)` | `13` | First addition request misses |
| Repeat `(2, 11)` | `13` | Addition miss count unchanged |
| `10 + 1`, IDs `(10, 1)`, twice | `0` both times | Coefficient wraparound; second request hits |
| `(Q, 0)` and `(0, Q)` | Throw `out_of_range` | Addition miss count unchanged |

After multiplication has cached `(2, 11)` as `22`, addition must still return
`13` for that same key. Addition must not change the multiplication miss count;
an existing multiplication hit must remain a hit. This checks table independence.

The implemented unary methods are `negate_id(id)` and `reciprocal_id(id)` in the same state.
Each operation takes one operand, so each table needs **Q entries**, rather than
Q squared. Two 1331-entry tables of 16-bit IDs use 5324 bytes of result storage
in total. Allocate them in the constructor, fill them with `EMPTY`, and initialize
separate negation and reciprocal miss counters to zero. Use local result variables.

```text
function negate_id(id):
    validate id against size(), throwing out_of_range if invalid
    if negation_results[id] != EMPTY:
        return negation_results[id]

    original_result = value(id).negate()
    result_id = encode<OriginalField>(original_result)
    negation_results[id] = result_id
    negation_misses += 1
    return result_id

function reciprocal_id(id):
    validate id against size(), throwing out_of_range if invalid
    if reciprocal_results[id] != EMPTY:
        return reciprocal_results[id]

    original_result = value(id).reciprocal()       // zero throws domain_error
    result_id = encode<OriginalField>(original_result)
    reciprocal_results[id] = result_id
    reciprocal_misses += 1
    return result_id
```

Expose read-only `negation_miss_count()` and `reciprocal_miss_count()` accessors.
Do not catch zero's reciprocal exception to return an ID or store a value. The
original method throws before the table write and counter increment; calling it
again must throw again and leave the miss counter unchanged. ID 0 has a perfectly
valid negation (ID 0), which must be cached normally.

Suggested explicit tests:

| Operation | Input ID | Expected result |
| --- | --- | --- |
| Negation | `0` | `0`, including a repeated cache hit |
| Negation | `2` | `9` |
| Negation | `11` (`x`) | `110` (`10x`) |
| Reciprocal | `1` | `1` |
| Reciprocal | `2` | `6`, since `2 * 6 = 1` modulo 11 |
| Reciprocal | `0`, twice | `domain_error` both times; no extra miss recorded |
| Either unary operation | `Q` | `out_of_range`; no extra miss recorded |

For each successful operation, call it twice and verify that its own miss count
increases only for the first request. Use a fresh state or record the count before
the check rather than assuming a result is still uncached. The other operation
counters must not change. Since there are only 1331 elements, also compare every
negation and every nonzero reciprocal ID with the encoded result from the original
catalog element. These loops give inexpensive exhaustive unary equivalence checks.

The following validation is now implemented in
[cache_state_tests.tpp](../tests/cache_state_tests.tpp). It uses a new local state
so every tested result entry starts empty; the shared-object identity checks
continue using the shared state:

```text
unary_state = new local State
Q = unary_state.size()

for id from 0 up to Q, excluding Q:
    original = read-only unary_state.value(id)
    expected = encode(original.negate())
    before = unary_state.negation_miss_count()
    check unary_state.negate_id(id) == expected
    check unary_state.negation_miss_count() == before + 1
    check unary_state.negate_id(id) == expected       // repeat
    check unary_state.negation_miss_count() == before + 1

    if id != 0:
        expected = encode(original.reciprocal())
        before = unary_state.reciprocal_miss_count()
        check unary_state.reciprocal_id(id) == expected
        check unary_state.reciprocal_miss_count() == before + 1
        check unary_state.reciprocal_id(id) == expected   // repeat
        check unary_state.reciprocal_miss_count() == before + 1

check unary_state.negation_miss_count() == Q
check unary_state.reciprocal_miss_count() == Q - 1

repeat twice:
    expect domain_error from unary_state.reciprocal_id(0)
    check unary_state.reciprocal_miss_count() == Q - 1
```

Here "new local State" means an ordinary local object, written `State unary_state;`
in C++. Its catalog contains all field values, but its result tables start empty
and its miss counters start at zero. It does not reset or share the result tables
returned by `state_for`. Keep the explicit known-value checks: they
independently explain expected mathematics, while the loop checks every ID and
both miss and hit paths.

The future public wrapper delegates to ID-level operations once they pass:

```text
function CachedElement.add(other):
    state = shared state for this field
    result_id = state.add_ids(this.id, other.id)
    return cached_element_from_known_id(result_id)

function CachedElement.negate():
    state = shared state for this field
    result_id = state.negate_id(this.id)
    return cached_element_from_known_id(result_id)

function CachedElement.reciprocal():
    state = shared state for this field
    result_id = state.reciprocal_id(this.id)
    return cached_element_from_known_id(result_id)

a - b = a.add(b.negate())
a / b = a.multiply(b.reciprocal())
```

## 9 Preserve construction and public compatibility

- [x] Delegate integer, coefficient, and polynomial construction to the original
  implementation before encoding the normalized result.
- [x] Preserve `from_index` bounds and enumeration semantics.
- [x] Preserve coefficient access, printing, and the polynomial reference API.
- [x] Preserve public aliases, field descriptors, operator return types,
  structure interfaces, and the public parent relationship.
- [x] Forward modulus selection to the original field, retaining its deterministic
  search and the same selected polynomial.

```text
from_integer(number):
    original = OriginalElement.from_integer(number)
    return cached_element_from_known_id(encode(original))

from_coefficients(coefficients):
    original = OriginalElement.from_coefficients(coefficients)
    return cached_element_from_known_id(encode(original))

from_polynomial(polynomial):
    original = OriginalElement.from_polynomial(polynomial)
    return cached_element_from_known_id(encode(original))

from_index(index):
    validate the original index bounds
    ensure field initialization
    return cached_element_from_known_id(index)

coefficient(degree): return original_values[this.id].coefficient(degree)
coefficients():      return original_values[this.id].coefficients()
representative():   return const reference to original_values[this.id].representative()
print():            print original_values[this.id]

PublicField.Element = CachedElement
CachedElement.parent_type = PublicField
PublicField.modulus():     return OriginalField.modulus()
PublicField.cardinality(): return OriginalField.cardinality()
```

Preserve `parent()` as the public field, and bind operator adapters to the cached
type. Merely inheriting the original element would leave adapters tied to its
original type. A polynomial reference must point into the stable catalog rather
than a temporary decoded element.

### Wrapper walkthrough

The wrapper converts the public element API to IDs; all hit/miss logic stays in
[cache_state.hpp](../dependencies/algebra-finite-fields-cached/include/algebra/finite_fields/detail/cache_state.hpp).
Keep the original public signatures as a checklist while implementing the bodies.

**Placement while developing.** Use the implemented `detail/cached_finite_field.hpp`
beside the existing internal headers. The replacement public `finite_field.hpp`
now includes this implementation header. The implementation
still declares the public classes in namespace `algebra`; a directory name does
not determine a C++ namespace.

This implementation filename lets a separate wrapper test executable include
it directly. The conversion test places the selected provider's public include
directory before the local helper directory, so its public type follows the
backend selection. The wrapper test includes this implementation and its private
original backend, without including the original public finite-field header in
that translation unit. Both providers use the same public class names; section
10 describes selection of the single public target.

The [staging header](../dependencies/algebra-finite-fields-cached/include/algebra/finite_fields/detail/cached_finite_field.hpp)
now defines both public classes and their method bodies. The walkthrough below
records their implementation. Bodies stay visible in the header because these
are templates. Conditional storage and non-cached branches are implemented and
tested with another field size.

**Comparison with the original.** Preserve signatures, return types, constness,
and behavior. The arithmetic algorithms remain in the private original backend.

| Methods | Relationship to the original body |
| --- | --- |
| Private tagged constructor | Same move-into-member pattern, with Storage instead of a polynomial |
| Default constructor | Validate the field as before; represent zero using the chosen storage |
| `from_known_id`, `wrap_original`, `original_value` | New private conversion/access helpers |
| Integer/coefficient/polynomial factories | Delegate to the corresponding original factory, then wrap |
| `from_index` | Keep bounds behavior; the cached branch stores the index directly |
| `zero`, `one` | Original bodies can remain unchanged |
| `add`, `multiply`, `negate`, `reciprocal` | Cached branch calls state methods; fallback calls original methods |
| `is_zero`, `is_one`, equality | Cached branch compares IDs; fallback uses original values |
| Coefficient access and representative | Obtain the original value through the catalog/helper, then delegate |
| `parent` | Original body can remain unchanged when parent_type names the new public field |
| Stream output | Print original_value(), preserving polynomial output rather than printing the ID |
| Public field constructor and element factories | Original body shapes remain unchanged, referring to the new Element alias |
| Public field modulus/initialization/cardinality | Forward to OriginalField; reuse its modulus construction and validation |

**Two public classes.** Retain the templates and signatures from
[original_finite_field.hpp](../dependencies/algebra-finite-fields-cached/include/algebra/finite_fields/detail/original_finite_field.hpp):

- `FiniteFieldElement<P, K, ModulusPolicy>` stores a value and performs operations.
- `FiniteField<P, K, ModulusPolicy>` describes the field and supplies factories.

Forward-declare the public field, define the element, then define the field, as
in the original. Keep the original default template arguments on the field's
forward declaration, without repeating them on its definition. Include the
internal original header and cache state outside your public namespace block.

**Aliases and storage.** These are conceptual names inside the element class:

```text
Self = public FiniteFieldElement<P, K, ModulusPolicy>
OriginalField = private original_fields.FiniteField<P, K, ModulusPolicy>
OriginalElement = OriginalField.Element
parent_type = public FiniteField<P, K, ModulusPolicy>
coefficient_type = OriginalElement.coefficient_type
polynomial_type = OriginalElement.polynomial_type
algebra = FieldDescriptor

cache_enabled = (P == 11 and K == 3)
Storage = compile-time choice:
    unsigned 16-bit ID, when cache_enabled
    OriginalElement, otherwise

private value_: Storage
```

In C++, `std::conditional_t` from `<type_traits>` expresses the storage choice,
and `if constexpr` expresses the branches below. Only one representation is
stored. Other field sizes retain original arithmetic and never allocate the dense
cache. Their public element type and parent relationship remain intact as well.
The original field type includes the modulus policy, so states remain separate
even for different policies with the same characteristic and degree.

Use fully qualified `::algebra::detail::original_fields::FiniteField<...>` and
`::algebra::detail::cached_fields::state_for<...>()` in the implementation. The
leading `::` matters inside a class whose public metadata includes a type alias
named `algebra`.

Inherit the three original operator adapters with **Self** as their template
argument: additive, multiplicative, and division operators. These implement
`+`, `-`, `*`, `/`, and assignment operators by calling your four methods. The
public field inherits `FieldStructure<Self>`. The element does not inherit the
original element or the field structure.

**Private helpers and default construction.** Separate public validation from
an internal constructor for an already-valid result:

```text
private constructor(value, KnownValue):
    initialize value_ directly by moving value into it

private from_known_id(id):
    return Self(id, KnownValue{})
    // Only used in the cached branch, after initialization and validity are known.

private wrap_original(original):
    if cache_enabled:
        return from_known_id(encode<OriginalField>(original))
    else:
        return Self(move original, KnownValue{})

public default constructor:
    OriginalField.initialize()
    if cache_enabled:
        value_ = 0
    else:
        value_ = OriginalElement.zero()
```

A private tag constructor can perform those direct constructions. It should not
call a public factory, decode an ID, or rebuild a polynomial. A cache hit already
has a valid ID. Keep the tag/helper private so callers cannot create invalid
elements. Default construction must still validate the original field and
represent zero.

Use a C++ member initializer list for the tagged constructor, following the
original tagged constructor's pattern. Its tag is an empty type that selects
the trusted constructor; the tag is not another stored data member. `move` in
the pseudocode corresponds to `std::move` from `<utility>`.

**Factories.** The original normalizes coefficients and reduces polynomials;
reuse that work before encoding:

```text
from_integer(number):
    return wrap_original(OriginalElement.from_integer(number))

from_coefficients(values):
    return wrap_original(OriginalElement.from_coefficients(values))

from_polynomial(polynomial):
    return wrap_original(OriginalElement.from_polynomial(polynomial))

from_index(index):                              // same uint64 input as original
    if index >= OriginalField.cardinality():
        throw out_of_range
    if cache_enabled:
        OriginalField.initialize()
        return from_known_id(convert index to 16 bits)
    else:
        return wrap_original(OriginalElement.from_index(index))

zero(): return default-constructed Self
one():  return from_integer(1)
```

Validate an index **before** narrowing it to 16 bits. These inputs must keep their
different meanings: `from_integer(11)` is zero, while `from_index(11)` is `x`.
Constructing from coefficients `{0, 0, 0, 1}` must reduce the cubic to `{7, 10, 0}`
before encoding. No factory should prefill operation results.

**Arithmetic.** Keep these methods `const` and return Self by value. A const
element may still request a result from the separate writable shared cache:

```text
multiply(other):
    if cache_enabled:
        state = state_for<OriginalField>() by reference
        return from_known_id(state.multiply_ids(value_, other.value_))
    else:
        return wrap_original(value_.multiply(other.value_))

add(other):
    if cache_enabled:
        state = state_for<OriginalField>() by reference
        return from_known_id(state.add_ids(value_, other.value_))
    else:
        return wrap_original(value_.add(other.value_))

negate():
    if cache_enabled:
        state = state_for<OriginalField>() by reference
        return from_known_id(state.negate_id(value_))
    else:
        return wrap_original(value_.negate())

reciprocal():
    if cache_enabled:
        state = state_for<OriginalField>() by reference
        return from_known_id(state.reciprocal_id(value_))
    else:
        return wrap_original(value_.reciprocal())
```

For example, multiplying elements containing IDs `121` and `11` obtains ID `117`
from the state and returns an element containing `117`. There is no second
encoding or polynomial construction on the hit path. Let the original/state
exception propagate for zero's reciprocal.

The element's `CacheState` alias names a type; it is not a state object. Obtain
the object with `::algebra::detail::cached_fields::state_for<OriginalField>()`
and bind it by reference. The operator adapters already implement the arithmetic
operators through these four methods, so their operator bodies need no copies.

**Views, predicates, and output.** Centralize access to the original value:

```text
private original_value() const -> const reference to OriginalElement:
    if cache_enabled:
        return state_for<OriginalField>().value(value_)
    else:
        return value_

coefficient(index): return original_value().coefficient(index)
coefficients():     return original_value().coefficients()
representative():   return const reference to original_value().representative()
stream output:      print original_value()

is_zero():
    if cache_enabled: return value_ == 0
    else: return value_.is_zero()
is_one():
    if cache_enabled: return value_ == 1
    else: return value_.is_one()
operator==(other):
    return value_ == other.value_                // works for both storage types

parent():
    static const parent_type instance            // the new PUBLIC field type
    return instance by const reference

friend stream_output(out, element):
    return out << element.original_value()
```

The shared catalog outlives the returned polynomial reference. Decoding to a
temporary and returning its polynomial reference would leave a dangling reference.
Delegating coefficient access also preserves the original out-of-range behavior.
Keep the original friend output signature and define its short body inside the
class. Printing `value_` directly in the cached branch would print an ID. Delegate
to the original element's stream operator to retain the original polynomial
format. Include `<ostream>` when adding that friend.

**The public field class.** Its Element alias names your new public element;
retain Base, PolynomialType, characteristic, and extension_degree. Most bodies
forward metadata or use the new element factories:

Give the public field its own private OriginalField alias with the same template
parameters; the element's private alias is not accessible from this class. Copy
the original public field's inheritance and metadata declarations, with Element
naming the new public element. Define this field class after the element class.
The field was only forward-declared in the initial scaffold; it is now defined
after the element in the staging header.

```text
constructor:           initialize()
initialize():          OriginalField.initialize()
modulus():             return OriginalField.modulus() by const reference
cardinality():         return OriginalField.cardinality()
element(integer):      return Element.from_integer(integer)
element(coefficients): return Element.from_coefficients(coefficients)
zero():                return Element.zero()
one():                 return Element.one()
alpha():               return Element.from_polynomial(PolynomialType.variable())
```

Initialization forwards to the original field rather than building cached
elements. The state constructor also uses the original field, which prevents
a cycle through public initialization.

**Implementation order.** First get storage, default construction, `from_index`,
the original-value view, coefficient access, equality, and the minimal public
field metadata working together. Next add the other factories, arithmetic, and
the remaining API. Keep original fallbacks in the compile-time branches. A
separate [cached_element_tests.tpp](../tests/cached_element_tests.tpp) now includes
the staging header directly and follows the existing CMake test-target pattern.

Check that `from_index(11)` gives `{0, 1, 0}`, `from_integer(11)` gives zero,
cubic construction reduces correctly, and arithmetic returns cached public
elements. Compare with the private original by coefficients, test parent/operator
types, and instantiate another field such as `<5, 2>` to check the fallback.
Only after this public layer works should the normal provider selection change.

## 10 Select one public provider through CMake

- [x] Give the replacement the same public headers, type names, and
  `algebra::finite_fields` target.
- [x] Select original or cached provider in
  [Dependencies.cmake](../cmake/Dependencies.cmake).
- [x] Keep matrix and determinant callers on their existing arithmetic interface.

The root [CMake configuration](../CMakeLists.txt) accepts these cache arguments:

```text
-DALGEBRA_ASSIGNMENT_BACKEND=original
-DALGEBRA_ASSIGNMENT_BACKEND=cached
```

Choose one value, configure, then build through CMake Tools. The default is
`original`; unrecognized values are rejected. The original branch fetches the
pinned upstream provider. The cached branch adds the
[local provider](../dependencies/algebra-finite-fields-cached/CMakeLists.txt),
which exports the same `algebra::finite_fields` interface target and links the
same pinned ring and polynomial dependencies. Shared enumeration, modulus, and
prime-field headers are unchanged upstream copies. Its public extension-field
entry header includes the cached wrapper. This keeps application source and
matrix/determinant arithmetic calls unchanged.

Both selections have been built and tested in Release mode, including the
seeded dimension-8 application reference. Timing builds were checked for
`-O3 -DNDEBUG -std=gnu++20`, absence of profiling flags, and the selected public
include directory. See [the measurement record](../results/backend-check/README.md)
for the initial runtime and memory results.

```text
if cached provider is selected:
    load the replacement finite-field dependency
else:
    load the pinned original finite-field dependency

load the unchanged matrix dependency
```

Load only one public provider per build. The retained internal original classes
belong to the replacement dependency and do not create another public provider.

## 11 Validate incrementally and measure consistently

- [x] Verify ID round trips for every element before building tables.
- [x] Check known ID operations on empty entries, then repeat to exercise hits.
- [x] Compare all unary ID results with the original implementation.
- [x] Compare representative binary operands, expanding to exhaustive checks where
  runtime permits; include legitimate zero results.
- [x] Check construction, bounds, and exceptional cases.
- [x] Run the seven existing determinant cases against original and cached elements.
- [x] Run seeded application reference comparisons after switching the provider.
- [x] Compare original and cached results by their coefficients, since the two
  element types differ.
- [x] Exclude temporary profiling instrumentation from ordinary timing builds.
- [x] Measure fresh-process initialization and cache filling as part of the
  cached workload; label any separate warm-cache experiments explicitly.

```text
start with an empty multiplication cache
calculate cached_two.multiply(cached_x)
check coefficients are [0, 2, 0]
check the request missed

repeat the same request
check coefficients are still [0, 2, 0]
check the request hit
```

Each fresh process starts with empty arithmetic tables. Benchmark runs must
retain that startup and miss cost. Warm-cache experiments can be added later
as a separately labeled measurement.
