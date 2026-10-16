# Price returns and trailing moving averages

The program reads a historical close-price series, calculates simple price
returns and a trailing simple moving average (SMA), and writes an aligned CSV.
The two SMA implementations produce the same mathematical quantity by different
algorithms. This is a data-processing example; it does not make a trading or
profitability claim.

See the [README](../README.md) for commands and the generated Doxygen API
documentation for function contracts and exceptions.

## Input and alignment

The input is a deliberately restricted `date,close` CSV format, not a general CSV
parser or a market-data feed. Dates must be valid Gregorian dates in `YYYY-MM-DD`
format and strictly increase. Close prices must be finite and strictly positive.
The reader accepts surrounding spaces and tabs, CRLF line endings, and an optional
UTF-8 BOM. It rejects quoted fields, extra columns, blank records, duplicate dates,
and invalid data instead of silently sorting or filling them.

There is no trading calendar, corporate-action adjustment, timezone conversion,
or missing-observation repair. Consecutive rows need not be consecutive calendar
days or trading sessions. The user must supply a consistent asset, currency, and
price basis. A window is a count of observations, not a number of elapsed days.

Write the prices as `p[0], ..., p[n-1]`. The output preserves all `n` dates and
prices. The first simple-return cell is empty. For an SMA window of `w`, the first
`w-1` SMA cells are empty; the first complete SMA belongs to row `w-1`. Missing
output cells at the beginning are empty CSV fields, not zero prices or zero
returns.

## Simple price returns

The return attached to row `i`, for `1 <= i < n`, is

```text
return[i] = p[i] / p[i-1] - 1
```

For prices `[100, 110, 99, 108]`, the returns are approximately
`[0.10, -0.10, 0.09090909]`. These are changes between adjacent observations,
expressed as fractions: `0.10` means 10%. They are not automatically daily returns
or total returns including dividends. A one-observation input has no return.

The formula uses the current and previous observations. It does not use a future
price. This alignment alone does not establish that the input data would have
been available at a particular historical trading time.

## Trailing SMA

The valid window range is `1 <= w <= n`. Let `q = n-w+1` be the number of complete
windows. SMA output element `k`, for `0 <= k < q`, is

```text
SMA[k] = (p[k] + p[k+1] + ... + p[k+w-1]) / w
```

The value belongs to input row `k+w-1`. Thus a trailing SMA at a row uses that
row and earlier rows, with no later observations.

For prices `[100, 110, 99, 108]` and `w = 3`:

```text
(100 + 110 + 99) / 3 = 103
(110 + 99 + 108) / 3 = 105.666666...
```

The aligned SMA column is `[empty, empty, 103, 105.666666...]`.

### Recompute each window

`sma_naive` validates the input and computes each window sum independently with
compensated summation. It scans `w` prices for each of the `q` outputs.

- Input validation costs `O(n)`.
- Window calculations cost `O(q*w)`.
- Total time is `O((n-w+1)*w+n)`.
- Returned output storage is `O(q)`; the sum itself needs constant extra space.

This implementation provides a direct reference for checking the rolling
algorithm. Agreement between two floating-point algorithms is not, by itself,
a proof that either equals the exact real-number answer.

### Maintain a rolling sum with two-stack aggregation

`sma_rolling` stores the current window in a two-stack aggregate queue. Each
stack node caches a compensated sum of the stack prefix ending at that node.
New prices are pushed onto the back stack. Old prices are popped from the front
stack; when the front is empty, all back elements are transferred in reverse
order, computing the front prefix sums along the way. The current total is
obtained by merging the two top aggregate states.

No departing large value is subtracted from a potentially much smaller window
sum. For example, after very large prices leave, a remaining window of `[1,1,1]`
still has its own saved aggregate instead of depending on a residual left by
large-number subtraction. The implementation also uses compensated summation
inside each aggregate.

Every observation is pushed to the back, transferred to the front, and removed
at most once. Each event performs constant work, so the complete calculation,
including validation, takes `O(n)` total time. The queue requires `O(w)` extra
workspace; returned output storage is `O(q)`.

This is an amortized bound. One transfer can cost `O(w)`; an individual output
step is not guaranteed to take worst-case constant time. Both algorithms can
still have floating-point rounding errors; the queue avoids a specific
catastrophic-cancellation mechanism rather than providing exact arithmetic.

At `w = 1`, both algorithms already perform linear total work. At `w = n`, there
is one output and both scan the input. Therefore the complexity difference does
not imply a speed advantage for every size and window.

## Floating-point limits

The implementation uses ordinary `double` arithmetic. Compensated summation and
subtraction-free queue aggregation reduce particular rounding errors; they do not provide a
universal accuracy guarantee, exact decimal arithmetic, or an error bound for
every finite positive input.

Invalid prices and invalid windows raise `std::invalid_argument`. Nonfinite
intermediate arithmetic or nonfinite results raise `std::overflow_error`. A
nonpositive computed SMA is also rejected as a numerical failure.
In particular, several finite positive prices may overflow a window sum even
when their exact mathematical mean would be finite; such input can be rejected.
Simple-return division can also overflow for extreme finite price ratios.

Results should be interpreted with their input precision and numerical scale in
mind. The program is not a substitute for a financial data-quality policy or a
specialized decimal accounting implementation.

## Benchmark comparison

`benchmarks/benchmark_core.cpp` uses the included Google Benchmark library to
compare `BM_SMA_Naive` and `BM_SMA_Rolling` at every pair from:

```text
n      = 1024, 16384, 65536
window = 1, 2, 3, 5, 10, 20, 100
```

There are 21 workloads per method, 42 in total. Both receive the same deterministic,
strictly positive synthetic price series near 100, with a repeating component
and small oscillations. The series is not a measured financial dataset.

Data generation takes place before the timed loop. Before timing either method,
the benchmark compares all outputs of both algorithms, checking lengths,
finiteness, and the sample-scale tolerance

```text
abs(naive[i] - rolling[i]) <= 1e-9 + 1e-11 * abs(naive[i])
```

An exception or mismatch calls `SkipWithError` and returns immediately. This
check is specific to the benchmark data and does not replace unit tests or give
a universal accuracy guarantee.

Every timed iteration calls one complete SMA function. Both timings include the
same input-validation contract and allocation and destruction of the returned
vector. The rolling algorithm's additional queue allocations and transfers are
also included; they are part of its actual algorithm cost. CSV parsing, file writing, data generation, and the preliminary
cross-check are excluded. The benchmark exposes input and result data pointers
with `DoNotOptimize` and uses `ClobberMemory` to discourage optimization that
would remove the measured work. The reported item count is input prices per
iteration, not output rows or arithmetic operations.

Windows 2, 3, 5, and 10 measure the region where direct summation can cost less
than maintaining a two-stack queue. Window 1 has a shared copy-only fast path;
its timing does not measure the queue. The same implementations are used for
all other windows, so a crossover is observed rather than imposed by a selector.

Run the `run_benchmarks` CMake target described in the README. Its console output
and `build/benchmark.json` contain the actual measurements and runtime context.
Compare the methods only for matching `n`, `window`, build configuration, and
machine. Repetitions help describe variation but do not remove noise from CPU
frequency changes, system load, or shared CI hardware. Allocation, validation,
and the small-window cases can dominate part of the cost.

The fixed set of benchmark inputs provides evidence for those runs. It does not
guarantee that the rolling implementation is faster in every workload. No timing
or speedup values are filled in before measurement.

The JSON check expects every registered algorithm/size/window name and, by
default, repetition indices 0, 1, and 2 exactly once for each name. Each raw
measurement must have a positive iteration count and valid positive CPU and
real times in a recognized unit. Missing raw measurements, duplicate indices,
unexpected workloads, and benchmark errors cause failure. Aggregate rows alone
are insufficient. Regression tests exercise these rejection cases through CTest.

A [local measured snapshot](benchmark-results.md) records one Release run, including the window-1 results and runtime limitations.
