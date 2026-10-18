# Sample data

The small sample and rejection fixtures are synthetic. A separate frozen ECB
snapshot provides an actual financial-data example.

- `sample_prices.csv`: four fictional positive close observations, in one arbitrary currency and with no corporate actions. The dates are calendar dates, not a claim about an exchange's trading calendar.
- `invalid_duplicate.csv`: deliberately repeats a date to demonstrate rejection.

The adjacent simple returns are `0.1`, `-0.1`, and `1/11`. For a window of three observations the valid SMA values are `103` and `317/3`. The first return and first two SMA output cells must be empty.

Supply only one asset, currency, and consistent price basis per file. With real data, record its source, currency, date range, and raw/adjusted price convention. The program does not itself adjust splits or dividends.

## Actual ECB reference-rate snapshot

- `ecb_eurusd_2025_01_source.csv`: original ECB CSV response with metadata.
- `ecb_eurusd_2025_01.csv`: 22 unchanged dates and reference rates, normalized to
  the program's two-column input contract.
- `ecb_eurusd_2025_01_provenance.json`: source URL, units, date range, retrieval
  date, transformation and SHA-256 hashes.

Source: ECB statistics. The series is `EXR.D.USD.EUR.SP00.A`, USD per EUR,
from 2 to 31 January 2025. These are reference rates, not exchange closing
prices. The column name `close` is an adapter to the existing program interface.
See the [real-data walkthrough](../docs/market-example.md) for reproduction,
calculated returns and SMA, attribution and interpretation.
