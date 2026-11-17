# Quick backend comparison

The backend selector and initial performance check are complete. Both configurations
pass all five CTest executables, and the dimension-8 reference agrees exactly.
The normal application source and its public includes/link-target names are unchanged.

Set these CMake cache arguments, then configure and build through CMake Tools:

```text
-DALGEBRA_ASSIGNMENT_BACKEND=original   # or cached
-DCMAKE_BUILD_TYPE=Release
-DALGEBRA_ASSIGNMENT_SIZE=64           # then 128, 256, and 512
-DALGEBRA_ASSIGNMENT_GPROF=OFF          # ordinary, uninstrumented timings
```

The default backend is `original`. Changing backend or size requires reconfiguration
and rebuilding. The measured configurations use Release and have profiling off.

## Results

Three fresh processes per case, seed 123456, same host/compiler and Release flags.
Times below are median application totals, including generation, the product,
all three determinants, and cache initialization/filling within that workload.
Speedup is original median divided by cached median. RSS is the largest peak
resident memory across the three runs.

| Dimension | Original (s) | Cached (s) | Speedup | Original RSS (MiB) | Cached RSS (MiB) |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 64 | 0.137322 | 0.124868 | 1.10x | 4.684 | 10.484 |
| 128 | 1.079465 | 0.496593 | 2.17x | 7.121 | 10.563 |
| 256 | 8.600025 | 0.881000 | 9.76x | 17.621 | 10.934 |
| 512 | 70.562422 | 3.442053 | 20.50x | 59.605 | 12.473 |

The cached backend helps more at larger tested dimensions. Its fixed arithmetic
tables increase memory at dimensions 64 and 128; compact matrix elements reduce
total memory below the original at dimension 256. The smallest timing difference
is modest, and these sizes do not establish an exact break-even point. The two
binary cache tables alone allocate 2 * 1331 * 1331 * 2 bytes (about 6.76 MiB);
their result slots begin empty. The smaller element IDs reduce matrix storage.

## Phase timing

| Dimension | Original product (s) | Cached product (s) | Original det sum (s) | Cached det sum (s) |
| ---: | ---: | ---: | ---: | ---: |
| 64 | 0.067776 | 0.067583 | 0.068851 | 0.056482 |
| 128 | 0.543091 | 0.365153 | 0.533891 | 0.127523 |
| 256 | 4.326935 | 0.764055 | 4.264297 | 0.107552 |
| 512 | 36.642643 | 2.623672 | 33.812704 | 0.786267 |

The cache is shared across phases. The product can populate entries that later
determinants reuse, so these determinant times describe their position in this
workload, not independent cold determinant runs. More cache filling during the
larger product is a plausible explanation for the cached determinant sum decreasing
from dimension 128 to 256; cache occupancy was not instrumented in these runs.

Each repetition launches a new process, so arithmetic caches are cold at process
start. GNU Time also includes program shutdown; the application's internal total
ends before static-cache destruction. GNU Time wall times are retained separately
and reported to hundredths of a second.

## Files and reproduction

- [summary.csv](./summary.csv): direct backend comparisons.
- [backend-summary.csv](./backend-summary.csv): medians, ranges, phase times, and memory per case.
- [runs.csv](./runs.csv): all 24 individual measurements and log filenames.
- [metadata.json](./metadata.json): build settings, source hashes, host, and validation.
- The `original-8-reference.log` and `cached-8-reference.log` files retain the smoke checks.

After building each selected backend/dimension, measure three separate invocations,
changing the backend, dimension, and run number in the filename:

```sh
/usr/bin/time -v ./build/src/algebra_determinants > results/backend-check/cached-256-run1.log 2>&1
```

Builds and test suites are outside the timed region. All measured runs returned
zero and matched the three known determinant coefficient arrays for their dimension.
Both backends were measured again here; comparisons do not mix these cached runs
with the earlier baseline collection.

The 512 measurements were collected afterward, three original runs followed by
three cached runs, with a 120-second timeout per run. Each original run took
approximately 70 seconds and peaked below 60 MiB; the cached runs took roughly
3.4 seconds and peaked below 13 MiB. The source hashes and Release flags matched
the earlier measurements. These are measurements on `louie`, not assignment-scale
extrapolations. See the [comparison figure](../../report/figures/backend-comparison.pdf)
and the separately instrumented [cached profile](../cached-profile/README.md).
