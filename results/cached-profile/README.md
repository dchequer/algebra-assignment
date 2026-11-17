# Cached profile, dimension 256

This profile is separate from the uninstrumented [timing runs](../backend-check/README.md).
The cached backend was built with GNU g++ 14.2.0, Release `-O3 -DNDEBUG`,
`-pg -g` when compiling and `-pg` when linking. It uses seed 123456, the same
field and dimension as the [original profile](../original/profile-metadata.json).
The instrumented executable is saved alongside the data so the profile can still
be examined after the normal build is restored.

Ten sequential fresh-process runs produced `run1/gmon.out` through
`run10/gmon.out`. Every run verified the determinant identity, matched the
original coefficients, and exited successfully. Each process starts with an
empty cache, then shares it across the product and the three determinants.
GNU gprof read all ten files together, combining their statistics into
[cached-256-profile.txt](./cached-256-profile.txt); the resulting flat profile
contains 7.82 seconds of sampled self time at a 0.01-second sampling interval.
Call counts and sampled seconds in the report are totals across ten processes.

The leading reported self-time shares are matrix multiplication 75.20%,
determinants 13.81%, and polynomial division/remainder 8.70%. The profile's
19,085,010 recorded calls to `divmod` are a total across all ten runs. Cache
misses still invoke original field arithmetic. Inlined functions are attributed
to enclosing functions, so the samples do not isolate cache lookup cost.
Shares describe each profile's own sampled time: the original and cached
percentages are not themselves a speedup measurement, and a zero sampled
time is not proof that a function costs nothing.

To reproduce the gprof reading from this folder:

```sh
gprof -b ./algebra_determinants run{1,2,3,4,5,6,7,8,9,10}/gmon.out > cached-256-profile.txt
```

The local gprof's `-s` option failed with `somebody miscounted` when asked to
write a combined `gmon.sum`. Passing all ten input files directly succeeded;
GNU gprof documents that multiple input files are summed for the report.
The executable was instrumented by configuring `ALGEBRA_ASSIGNMENT_GPROF=ON`;
the option defaults to `OFF` and should stay off for ordinary timing runs.
The separate [cached profile figure](../../report/figures/cached-profile.pdf)
uses the data in [profile-hotspots.csv](./profile-hotspots.csv).
