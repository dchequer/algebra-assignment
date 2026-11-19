# Finite field determinants solo Kanban

Implement a compatible cached finite field, validate it against the original,
then distribute determinant computation with MPI and report the measurements.
All cards have one owner: you. The requirements come from the
[assignment](./README.md#assignment); team coordination is omitted.

**Estimated effort:** 41–68 focused hours, plus benchmark runtime and any time
needed to obtain machine access. This is a planning estimate assuming familiarity
with C++ and basic MPI. Learning either can add substantial time. The provided
Apple M3 Max timings are reference measurements, not promised results on your
hardware. Smaller test sizes are appropriate while developing.

## Board

Move a card link between columns as its status changes. Keep at most **one** card
in progress. Move it to Review when its checklist is complete, then to Done after
checking its results yourself. Move blocked work to Blocked and note what it needs.
Start a Backlog card only when its listed dependencies are done.

| Backlog | Ready | In progress | Blocked | Review | Done |
| --- | --- | --- | --- | --- | --- |
| [09 MPI design](#09-mpi-design) | — | [08 Cached measurements](#08-cached-measurements) | — | — | [01 Machine access](#01-machine-access) |
| [10 MPI implementation](#10-mpi-implementation) | | | | | [02 Dependency map](#02-dependency-map) |
| [11 MPI correctness](#11-mpi-correctness) | | | | | [03 Small reference cases](#03-small-reference-cases) |
| [12 MPI break-even measurements](#12-mpi-break-even-measurements) | | | | | [04 Original measurements](#04-original-measurements) |
| [13 IEEE report](#13-ieee-report) | | | | | [05 Cache design](#05-cache-design) |
| [14 Presentation](#14-presentation) | | | | | [06 Cached dependency](#06-cached-dependency) |
| [15 Submission check](#15-submission-check) | | | | | [07 Cache equivalence tests](#07-cache-equivalence-tests) |

Keep this board in the repository as the source of truth for solo planning.

## Cards

### 01 Machine access

**Estimate:** 1–2 hours. **Depends on:** nothing.

- [x] Check this host, `louie`, as the primary development and sequential
  benchmark environment; its compiler and CMake versions meet the project floor.
- [x] Verify SSH access to `d0cheq01@mpi.nathan.cse.louisville.edu` and distinguish
  the login node from the Slurm worker nodes.
- [x] Identify the existing `sbatch` launch method, worker nodes, and current
  partition limits. Previous course notes show eight ranks on four worker nodes.
- [x] Agree to develop on `louie` and run distributed MPI measurements across
  course workers. Worker preparation and a fresh launch check are tracked in
  card 09, before the MPI implementation.

**Working arrangement:** use `louie` for development, correctness tests, and
sequential benchmarks. Use the course workers
for the distributed measurements required by the assignment, repeating the
cached sequential baseline on the same worker hardware for a fair MPI
comparison. The assignment does not name a required cluster provider. Remaining
cluster setup can wait until the sequential implementation has been improved
and measured.

Local host check on 2026-11-16:

| Item | Observed value |
| --- | --- |
| Hostname | `louie` |
| CPUs visible to the account | 4 |
| Memory | About 15 GiB total, 12 GiB available at check time |
| C++ compiler | GCC 14.2.0; application and determinant tests build in Release |
| CMake | 3.31.6; meets the project's 3.24 minimum |
| MPI | `mpirun` and `mpicxx` not found on PATH |

Course cluster check on 2026-11-16:

| Item | Observed value |
| --- | --- |
| Login | `ssh d0cheq01@mpi.nathan.cse.louisville.edu` succeeded |
| Login hostname | `ip-172-31-90-167` |
| Login resources | 2 CPUs, 911 MiB total RAM; worker resources are separate |
| Login tools | Open MPI 4.1.7, GCC 11.5.0, CMake 3.22.2 |
| Scheduler and partition | Slurm; `demo` |
| Worker nodes | `demo-dy-workers-[1-4]` |
| Configured resources per worker | 2 CPUs, 3891 MB memory reported by Slurm |
| Job time limit | 1 minute by default; 5 minutes maximum |
| Existing course launch method | `sbatch mpi_pi.slurm`; script loads Open MPI and runs `mpirun` |

Use the login node to prepare and submit jobs; run cluster computations on the
allocated workers. Begin with small matrices even on the larger local host,
then choose benchmark sizes from measured runtime and peak memory. Record local
and worker hardware separately when reporting cross-environment results.

### 02 Dependency map

**Estimate:** 2–3 hours. **Depends on:** nothing.

- [x] Inspect the pinned dependencies in
  [Dependencies.cmake](./cmake/Dependencies.cmake) and trace the finite field's
  arithmetic through its dependencies. Record the actual dependency chain and
  the public interface used by the matrix library and
  [main.cpp](./src/main.cpp).
- [x] Identify the class to wrap, its initialization and deterministic
  irreducible-polynomial search, and the header paths, type names, and CMake
  targets that the replacement must preserve.

Result: [dependency map and interface inventory](./docs/dependency-map.md).
Wrap `FiniteFieldElement`, retaining the `FiniteField::Element` entry point.
The matrix implementation is generic; the application contains the determinant
algorithm. Preserve the original modulus search and arithmetic on cache misses.

### 03 Small reference cases

**Estimate:** 2–4 hours. **Depends on:** 02.

- [x] Build the original in Release mode at a small matrix size, such as 8 or 32,
  and save the seed, modulus, determinant coefficients, and identity result.
  The current default size is 4000; change the CMake size setting for development.
- [x] Establish repeatable reference fixtures or a separate original executable
  for comparison. Include zero/identity matrices, a singular matrix, and a
  matrix that needs a pivot-row swap with a known determinant.
- [x] Establish a unit-test target and verify the reference cases before changing
  the field implementation.

Original reference run, retained from the output before extracting the
determinant function: dimension 8, seed 123456, field order 1331, modulus
`4 + x + x^3`, `det(A)` coefficients `[6, 2, 0]`, `det(B)` coefficients
`[4, 3, 2]`, `det(C)` coefficients `[8, 0, 7]`, identity verified `true`.
The user confirmed the same values after extraction into the shared header.

[determinant_tests.tpp](./tests/determinant_tests.tpp) contains seven cases:
zero, identity, singular, row swap, extension field entry, non-unit pivot,
and determinant multiplicativity. Each also checks that the input is unchanged.
The Release build and the registered CTest test passed on 2026-11-16.

### 04 Original measurements

**Estimate:** 2–3 hours. **Depends on:** 03.

- [x] Measure original Release runtime and peak resident memory at several
  manageable sizes, increasing gradually. Save raw results and hardware,
  compiler, flags, thread count, revision, size, and seed.
- [x] Profile a representative run to locate expensive arithmetic. Keep matrix
  multiplication, individual determinants, and whole-program time separate;
  [main.cpp](./src/main.cpp) already prints phase timings. Save graph inputs.

Nine original runs at sizes 64, 128, and 256 have been validated: all exited
successfully, verified the determinant identity, and repeated the same
determinant values for each size. See the [per-run data](./results/original/runs.csv),
[summary](./results/original/summary.csv), and [metadata](./results/original/metadata.json).
The [report section](./report/sections/original-baseline.tex) records the user's
build and timing procedure, statistics, phase timings, and function-level profile.

The optimized, instrumented dimension-256 run verified the identity. Its flat
profile attributes 49.71% of sampled self time to matrix multiplication, 35.19%
to the determinant function, and 12.13% to polynomial `divmod`, with 50,233,382
recorded calls to `divmod`. Inlined field arithmetic can be attributed to its
callers, so these are not isolated costs of the matrix and determinant loops.
See the [figure](./report/figures/original-profile.pdf),
[graph inputs](./results/original/profile-hotspots.csv),
[full flat-profile data](./results/original/profile-flat.csv), and
[profile metadata](./results/original/profile-metadata.json).

The temporary `-pg -g` compile options and `-pg` link option have been commented
out in [the executable's CMake configuration](./src/CMakeLists.txt). Rebuild
before ordinary benchmark measurements. Keep the instrumented profile separate
from the nine-run baseline.

### 05 Cache design

**Estimate:** 2–3 hours. **Depends on:** 02, 04.

- [x] Specify how elements and operand pairs become cache keys, which operations
  to cache, and how a miss calls the existing arithmetic. Preserve the original
  modulus, public behavior, zero-division behavior, and initialization rules.
- [x] Compare a simple `std::unordered_map` approach with compact element IDs or
  lookup tables if useful. There are 1331 elements and 1,771,561 ordered pairs;
  account for container overhead and initialization cost before choosing.
- [x] Decide how CMake will select original versus cached dependency while
  retaining the interface used by existing callers. Keep the caching change
  confined to the selected dependency.

Accepted: [cached field design and detailed checklist](./docs/cache-design.md).
Use compact IDs, a shared original-value catalog, and lazily filled dense
addition, multiplication, negation, and reciprocal tables. Retain the original
arithmetic internally for misses and preserve the public provider interface.

### 06 Cached dependency

**Estimate:** 5–8 hours. **Depends on:** 05.

ID-conversion validation is complete in
[element_id.hpp](./dependencies/algebra-finite-fields-cached/include/algebra/finite_fields/detail/element_id.hpp).
The user implemented both helpers and reports the registered
[conversion test](./tests/encode_decode_tests.tpp) passing all 1331 valid-ID round
trips, six known mappings, and three invalid-ID exception checks.

The [retained original field](./dependencies/algebra-finite-fields-cached/include/algebra/finite_fields/detail/original_finite_field.hpp)
differs from upstream only in its enclosing namespace. Modulus, coefficient,
and encoding comparisons against the public field pass for all 1331 IDs;
both registered tests build and pass.

The known reciprocal and cubic-product checks now exercise internal elements
and pass. The original implementation is ready as the arithmetic backend.

The [original-value catalog](./dependencies/algebra-finite-fields-cached/include/algebra/finite_fields/detail/cache_state.hpp)
and shared accessor are implemented. The [catalog test](./tests/cache_state_tests.tpp)
passes size, all 1331 value-to-ID mappings, shared identity, and bounds checks.
All three registered CTest targets pass.

The multiplication ID cache is implemented, and the user reports all tests pass.
The [state tests](./tests/cache_state_tests.tpp) cover a miss and hit for `2 * x`,
a cached zero, cubic reduction to ID 117, and an invalid left operand. The shared
state is now writable while catalog access remains read-only. Remaining
multiplication checks: identity, reversed operands, an invalid right operand,
and an unchanged miss count after rejected requests. The cubic-product comments
now consistently describe `-x - 4`.

The separate addition ID cache is implemented, and the user reports all tests
pass. Tests cover `2 + x`, repeated hits, coefficient wraparound and cached zero
for `10 + 1`, invalid-left-ID rejection without an extra miss, and independence
from the previously populated multiplication cache. Invalid-right-ID rejection
remains to be checked for addition as well.

The unary negation and reciprocal caches are implemented with independent tables
and miss counters. The [state test](./tests/cache_state_tests.tpp) now creates an
independent local state and compares all 1331 negations and 1330 nonzero
reciprocals against original arithmetic. Every request is repeated to verify a
hit without an extra miss. Two zero-reciprocal requests throw without increasing
the reciprocal miss count, and unary work leaves binary miss counters at zero.
All three registered CTest targets pass after rebuilding these tests.

The public field and element wrappers are implemented in the
[staging header](./dependencies/algebra-finite-fields-cached/include/algebra/finite_fields/detail/cached_finite_field.hpp).
Integration fixes removed the duplicate storage alias, corrected namespace-level
shared-state calls, completed the element declaration, moved trusted storage,
and preserved modulus validation in `from_index` before wrapping its ID.

The [wrapper tests](./tests/cached_element_tests.tpp) verify two-byte cached
elements, factories, cache hits/misses, all enumerated and unary values,
representative binary operands, inherited operators, polynomial views, output,
parent/field interfaces, exceptions, and the `<5, 2>` original-storage fallback.
The seven existing determinant fixtures also pass with the cached element type
through a second target built from [the same test source](./tests/determinant_tests.tpp).
All five registered CTest targets pass; edited code has no reported diagnostics.

Provider integration is complete. `ALGEBRA_ASSIGNMENT_BACKEND` selects `original`
(the default) or `cached`, preserving public includes and `algebra::finite_fields`.
The local replacement has its own interface target and public entry headers;
its shared helper headers are unmodified copies of the pinned upstream revision.
The application source did not change. Both Release configurations pass all five
tests and match the seeded dimension-8 reference. See
[section 10](./docs/cache-design.md#10-select-one-public-provider-through-cmake).

- [x] Implement the wrapper and cache selected operations, reusing the original
  arithmetic on misses and its deterministic polynomial selection.
- [x] Make the original sequential caller and matrix dependency compile against
  either implementation through the CMake dependency choice, with the same
  header paths and type names. Keep MPI work separate from this change.

### 07 Cache equivalence tests

**Estimate:** 3–5 hours. **Depends on:** 03, 06.

- [x] Compare exact results with the original implementation using fixtures or
  separate executables: conversions, equality, addition, negation,
  multiplication, reciprocals, and supported division behavior. Cover all 1331
  elements for unary operations and representative operand pairs, expanding to
  exhaustive binary checks where runtime permits. Exercise misses and hits.
- [x] Compare matrix products and determinant coefficients for the same small
  matrices, including singular and row-swap cases. Verify
  `det(A * B) = det(A) * det(B)` as an additional check.
- [x] Confirm that initialization chooses exactly the same modulus and that
  exceptional cases behave consistently with the original.

The [wrapper checks](./tests/cached_element_tests.tpp) compare matrix products
entry by entry as well as scalar operations. Both provider selections pass the
full five-target suite. The dimension-8 smoke runs and all timed dimensions
match the original determinant coefficients; see the
[validation metadata](./results/backend-check/metadata.json).

### 08 Cached measurements

**Estimate:** 3–5 hours. **Depends on:** 04, 07.

- [x] Repeat the original measurements on the same inputs, hardware, Release
  settings, and one CPU thread. State how cold cache, warm cache, and cache
  initialization are timed; include the cache in peak memory.
- [x] Produce runtime, speedup, and peak-memory comparisons plus informative
  profiling graphs. Investigate regressions before moving to MPI. Add larger
  sizes only when their expected runtime and memory fit your budget.

The [backend comparison](./results/backend-check/README.md) records 24
fresh-process runs: both implementations at dimensions 64, 128, 256, and 512, three
repetitions each, using the same Release flags and seed. All runs match reference
coefficients and verify the determinant identity. Median application speedups
are 1.10x, 2.17x, 9.76x, and 20.50x. Cached peak RSS is higher for the two smaller sizes
and lower at dimensions 256 and 512. The fixed table cost and cross-phase cache reuse are
documented, along with raw logs, per-run data, and source/build metadata.

The [comparison plots](./report/figures/backend-comparison.pdf) and
[cached profile](./results/cached-profile/README.md) are included in the
[report section](./report/sections/cached-comparison.tex). The 512 runs fit the
local time and memory budget; larger inputs are still unmeasured. The fixed cache
table cost explains the small-input memory increase, while the profile identifies
matrix multiplication as the largest sampled cached hotspot. The next increment
is the MPI design and a small worker-node access check.

### 09 MPI design

**Estimate:** 2–4 hours. **Depends on:** 01, 08.

- [x] Install a local MPI development environment and run the hello-world with
  four ranks on `louie`.
- [x] Provide CMake 3.24 or newer for cluster builds; CMake 3.31.6 was installed
  in the user's cluster account and used to build the cluster executable.
- [x] Verify a fresh small MPI job on at least two distinct worker nodes through
  Slurm, retaining the job script and output. Multiple ranks on one host are
  useful for debugging but do not establish a multi-machine run.
- [ ] Choose MPI benchmark sizes that fit the five-minute `demo` job limit, or
  establish access to adequate runtime before scheduling larger measurements.
- [ ] Design a row-distributed PLU/elimination determinant using the cached field.
  Specify nonzero pivot selection, pivot ownership, cross-rank row swaps, swap
  sign, pivot-row broadcasts, and local trailing-row updates.
- [ ] Specify a portable element encoding and ensure every rank uses the same
  modulus. Define collective handling of singular matrices so no rank exits
  while others wait for communication.
- [ ] Define benchmark boundaries: identical input matrices, per-rank cache
  preparation, setup/distribution, and determinant time including its required
  communication. Plan to report the slowest rank's elapsed determinant time.

Slurm job 89 ran four ranks across `demo-dy-workers-1` and `demo-dy-workers-4`.
The [job script](./mpi-hello.slurm) and [output copied from the user's terminal](./results/mpi-hello/job-89-output.txt)
record the check. The tiny integer-matrix distribution was also verified on
`louie` and across two workers in [job 97](./results/mpi-hello/job-97-output.txt),
whose output is copied from the user's terminal; see its
[job script](./mpi-distribute.slurm). A field-element version encoded and
reconstructed elements across two workers in [job 98](./results/mpi-hello/job-98-output.txt),
also copied from the user's terminal. Next: implement and test pivot selection
and sharing before elimination.

### 10 MPI implementation

**Estimate:** 6–10 hours. **Depends on:** 09.

- [ ] Add an MPI determinant executable or target while preserving the original
  and cached sequential benchmarks.
- [ ] Implement distribution, pivot selection and exchanges, broadcasts, local
  elimination, and final determinant assembly with correct row-swap parity.
- [ ] Log matrix size, seed, rank and machine counts, cache/setup costs, and
  determinant timing. Begin with tiny matrices on one machine.

### 11 MPI correctness

**Estimate:** 3–5 hours. **Depends on:** 07, 10.

- [ ] Match exact cached sequential determinants with one and multiple ranks;
  include singular matrices, pivots owned by another rank, cross-rank row swaps,
  and sizes that do not divide evenly among ranks.
- [ ] Run across at least two machines and verify identical field encoding,
  modulus, and inputs. Check that singular cases terminate cleanly on all ranks.
- [ ] Retain repeatable commands and correctness results before long benchmarks.

### 12 MPI break-even measurements

**Estimate:** 3–5 hours. **Depends on:** 08, 11.

The [benchmark plan](./docs/mpi-benchmark-plan.md) records worker probe 114,
the job limits, proposed rank layouts, and timing and cache-state boundaries.

- [ ] Establish the cached sequential baseline on one worker of the same type
  used for MPI. Keep local-host sequential results separately labeled so the
  reported MPI speedup uses a comparable hardware baseline.
- [ ] Compare cached sequential and MPI determinant runtimes on exactly the same
  matrices at increasing sizes and selected rank/machine counts. Exclude matrix
  multiplication from determinant comparisons; report setup separately and
  include communication needed by the determinant.
- [ ] Repeat feasible runs, report the timing summary and variability, and plot
  runtime/speedup against size. Record hardware and rank placement so hardware
  differences are visible.
- [ ] Report the smallest **tested** size where MPI is faster for each tested
  configuration, or state that no break-even was observed. Record communication
  overhead and per-rank/aggregate memory where practical.

### 13 IEEE report

**Estimate:** 4–6 hours. **Depends on:** 12 for final results.

- [x] Start the course IEEE LaTeX template in [report.tex](./report/report.tex).
- [ ] Explain the selected class, actual dependency chain, cache design,
  unchanged modulus, and equivalence tests in your own words.
- [ ] Add before/after runtime and peak-memory measurements, profiling graphs,
  the MPI algorithm and communication choices, and tested break-even results.
- [ ] Explain limitations and AI assistance, retain reproducibility details,
  and check that every result comes from saved measurements.

### 14 Presentation

**Estimate:** 2–3 hours. **Depends on:** 13.

- [ ] Prepare PowerPoint or Beamer slides covering the problem, cache design,
  correctness evidence, sequential gains, MPI approach, and break-even results.
- [ ] Rehearse explaining the arithmetic, pivoting, cache choices, and
  communication costs without relying on AI-generated explanations. Fit the
  assigned presentation duration.

### 15 Submission check

**Estimate:** 1–2 hours. **Depends on:** 13, 14.

- [ ] Verify documented builds and correctness tests from a clean build directory
  for original, cached, and MPI targets; avoid repeating expensive benchmarks
  unless results or code changed.
- [ ] Check the final report PDF, presentation, raw data and graphs, dependency
  revisions, build/run instructions, and the local Kanban board.
- [ ] Review the course submission instructions and submit the required files.

## Suggested order and scope

Resolve machine access first while inspecting the dependency interface. Finish a
small original-versus-cached correctness comparison before spending time on
large matrices. Complete and measure the sequential optimization before building
MPI, as the assignment requests. Start report notes and save raw data throughout.

If time gets tight, reduce the number of benchmark sizes and keep the MPI
distribution simple. Preserve the required correctness evidence, multiple-machine
run, profiling graphs, report, and presentation. A measured result showing that
MPI did not beat the sequential version is still a result the assignment allows.
Treat alternative cache designs, elaborate MPI optimizations, and full-size
4000/6000 runs as follow-up work when time and hardware permit.
