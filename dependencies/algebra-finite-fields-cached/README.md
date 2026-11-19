# Cached finite-field provider

This header-only dependency exports `algebra::finite_fields` and the same public
headers and types as the original provider. Select it at the assignment root
with the CMake cache setting `ALGEBRA_ASSIGNMENT_BACKEND=cached`; `original`
selects the pinned upstream implementation.

The public entry header forwards to [the cached implementation](include/algebra/finite_fields/detail/cached_finite_field.hpp).
For `<11, 3>`, elements store 16-bit IDs and arithmetic uses shared lazy caches.
Other field sizes retain original-element storage and arithmetic.

- `enumeration.hpp`, `modulus.hpp`, and `prime_field.hpp` are unmodified copies
  of the original files.
- [The original extension-field implementation](include/algebra/finite_fields/detail/original_finite_field.hpp)
  retains the upstream mathematical bodies in a private namespace for misses
  and normalization.
- Ring and polynomial dependencies use the same pinned revisions as upstream.
