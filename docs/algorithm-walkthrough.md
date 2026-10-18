# Following the SMA implementation

This walkthrough follows `src/indicators.cpp`. The [algorithm reference](algorithms.md)
defines the input contract and output alignment; the [usage guide](usage.md)
provides commands. These notes support reading and checking the code.

## First calculate the answer directly

For prices `[100, 110, 99, 108]` and window `3`, the complete windows are:

```text
[100, 110,  99] -> 309 / 3 = 103
[110,  99, 108] -> 317 / 3 = 105.666666...
```

The output SMA column is `[empty, empty, 103, 105.666666...]`. Returns are a
separate calculation: `110/100-1 = 0.10`, `99/110-1 = -0.10`, and
`108/99-1 = 1/11`. An SMA is in the same unit as the prices; a return is a fraction.

`sma_naive` performs the two window scans independently. It does not reuse
the first window when calculating the second one. For a general input with
`n` prices and window `w`, it visits `w` prices for each of `n-w+1` complete
windows, in addition to validating all `n` prices.

## Read the two stacks in the correct direction

The rolling implementation uses a queue with two vectors named `front_` and
`back_`. In every stack below, **the bottom is on the left and the top is on the
right**. `vector::back()` accesses that rightmost entry.

Each entry stores a price and a cached aggregate, not just the price. Write an
entry as `price:total` here. Its total includes that entry and everything to its
left in the same stack. Internally the aggregate has two fields, `sum_` and
`correction_`; all corrections in this small integer example are zero.

| Operation | Front: bottom to top | Back: bottom to top |
|---|---|---|
| Push 100 | `[]` | `[100:100]` |
| Push 110 | `[]` | `[100:100, 110:210]` |
| Push 99 | `[]` | `[100:100, 110:210, 99:309]` |
| Read first mean: `309/3` | `[]` | `[100:100, 110:210, 99:309]` |
| Transfer back top 99 | `[99:99]` | `[100:100, 110:210]` |
| Transfer back top 110 | `[99:99, 110:209]` | `[100:100]` |
| Transfer back top 100 | `[99:99, 110:209, 100:309]` | `[]` |
| Pop front top 100 | `[99:99, 110:209]` | `[]` |
| Push incoming 108 | `[99:99, 110:209]` | `[108:108]` |
| Read second mean: `(209+108)/3` | `[99:99, 110:209]` | `[108:108]` |

`queue.pop()` performs the transfer only when the front stack is empty. Reversal
makes the oldest price, 100, the new front top. Removing that entry reveals the
previously cached aggregate 209 for the remaining prices, 99 and 110. The queue
then receives 108. Its logical order is `[110, 99, 108]`: front top to bottom,
followed by back bottom to top.

The total is obtained from the two top aggregates, 209 and 108; the code does
not add every cached total in either stack. In the actual implementation,
`mean()` copies the front top aggregate and merges both fields of the back top
aggregate into it before dividing by the window size. If the front is empty,
it uses the back top directly.

## Why a transfer does not make total work quadratic

Consider the complete lifetime of one price:

1. It is pushed onto the back stack once.
2. It can move from back to front once. It never moves back again.
3. It can be popped from the front once. Some prices remain when processing ends.

Each entry insertion, transfer, removal, and top-aggregate merge uses a fixed number of
arithmetic operations. Across `n` prices there are at most `n` initial pushes,
`n` transfers, and `n` final pops, plus at most `n` mean queries. Validation
also scans the input once. Their total is therefore `O(n)`.

A single pop can transfer as many as `w` prices and cost `O(w)`; the constant
cost is **amortized**, meaning averaged over the sequence of operations. Each
stack holds at most `w` entries and each vector reserves `w` entries, so auxiliary
capacity is `O(w)`. The returned means additionally need `O(n-w+1)` storage.
Both functions receive the entire input vector and return a vector of means.

## What the rounding correction stores

Both SMA implementations use the same Neumaier compensated sum. With main sum
`s`, correction `c`, and incoming value `x`, the code follows this order:

```text
t = s + x
if abs(s) >= abs(x):
    error = (s - t) + x
else:
    error = (x - t) + s
c = c + error
s = t
```

The implementation evaluates the two operations forming `error` separately
and checks each intermediate value for finiteness. The final total is `s+c`.
Merging another aggregate means adding its main sum and then its correction;
it does not discard the other correction.

On the usual IEEE binary64 arithmetic with rounding to nearest, the spacing
around `1e16` is 2. An ordinary addition `1e16 + 1` rounds to `1e16`. Consider
the positive inputs `[1e16, 1, 1]`:

| Added value | Main sum `s` | Correction `c` |
|---:|---:|---:|
| `1e16` | `1e16` | `0` |
| `1` | `1e16` | `1` |
| `1` | `1e16` | `2` |

The final `s+c` is the representable value `10000000000000002`. Plain
left-to-right summation loses both unit additions. This explains the purpose
of the correction; it does not establish an error bound for every input.

The two-stack design addresses a related sliding-window problem. For
`[1e16, 1, 1, 1]` with `w=2`, an ordinary running total initially rounds to
`1e16`. Subtracting the departing `1e16` and adding the next `1` gives total
`1`, although the new window `[1,1]` has total `2`. The queue instead reveals
the saved aggregate for the retained `1` and merges the incoming `1`, giving
mean `1`. The unit test with this input checks both SMA implementations
against independently specified answers `[5e15, 1, 1]`.

The correction does not make arbitrary floating-point arithmetic exact.
Different addition orders may still yield different rounded answers, and
overflow is rejected. This is why tests use independent expected results and
appropriate tolerances as well as comparing the implementations.

## Why the simpler algorithm can win

`O(n)` describes total growth, not a timing advantage for every window. When
`w` is a small fixed number, the naive method is also linear in `n`, with
roughly `w` additions per output. The queue has entry storage, aggregate copies,
transfers, and merges. Those costs can outweigh a few direct additions.

The [measured 2026-10-16 Release snapshot](benchmark-results.md) gives this
comparison at `n=65536`, using medians of three CPU-time measurements:

| Window | Naive time (microseconds) | Rolling time (microseconds) | Faster method |
|---:|---:|---:|---|
| 2 | 324.506 | 704.775 | Naive |
| 5 | 627.977 | 982.621 | Naive |
| 10 | 1337.400 | 1109.840 | Rolling |
| 100 | 27440.000 | 1264.000 | Rolling |

The sampled winner changes between windows 5 and 10 in that run; windows 6-9
were not measured. This is evidence for that machine and workload, not an
automatic algorithm-selection threshold. Window 1 has a shared validation-and-copy
fast path. At window `n` there is only one mean, so both algorithms scan the
input. Both methods remain available through `--algorithm naive|rolling`.

## Questions a reviewer might ask

| Question | Answer |
|---|---|
| Why are the first two SMA cells empty for window 3? | Fewer than three observations are available. Zero would be an invented value. |
| Does a window of 3 mean three calendar days? | It means three observations; gaps between dates are permitted. |
| Why does transfer reverse the stack? | It places the oldest back element on top of the front stack, preserving queue order. |
| Why cache an aggregate in every entry? | A pop reveals a saved aggregate for the retained values, without subtracting the departing price from a rounded total. |
| Is every rolling step constant time? | No. A transfer can cost `O(w)`; the whole calculation is `O(n)` by the lifetime count above. |
| Why keep the naive implementation? | It is simple to inspect, provides an independent algorithm, and was faster for the measured small windows. |
| Are two matching implementations enough to prove correctness? | No. Tests also use hand calculations, an independently accumulated reference, invalid inputs, and numerical stress cases. |
| Is a higher price than SMA a proven trading signal? | No. The program calculates historical indicators; strategy evaluation would require separate execution assumptions, costs, and out-of-sample testing. |

To check understanding, explain the second window without looking at the table:
which element leaves, why the retained total is 209, where 108 is stored, and
why each observation can be transferred only once.
