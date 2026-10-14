# Extending the market-data tool

The current project validates one historical positive-price series and computes adjacent simple price returns and trailing SMA.

Keep financial semantics explicit when adding features:

1. Add new indicator contracts to include/course/core.hpp and implementation to src/indicators.cpp.
2. Keep CSV parsing and diagnostics in src/core.cpp.
3. Add independently checkable tests, boundary cases, and any new numerical limits.
4. Benchmark comparable algorithms on the same task, including the same validation and allocation costs.
5. Update output alignment, README, API comments, and sample expectations together.
6. Run bash scripts/verify.sh before delivery.

A second data-source adapter is a possible later extension. Normalize each source independently and record asset, currency, date range, and adjustment convention before alignment. Conflicting values must not be silently averaged or overwritten.

Trading strategies, transaction-cost models, corporate-action adjustment, and exchange calendars need separate specifications; they are not implied by the existing SMA output.
