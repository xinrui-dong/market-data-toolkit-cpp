# Sample data

Both CSV files are synthetic test fixtures, not actual market observations.

- `sample_prices.csv`: four fictional positive close observations, in one arbitrary currency and with no corporate actions. The dates are calendar dates, not a claim about an exchange's trading calendar.
- `invalid_duplicate.csv`: deliberately repeats a date to demonstrate rejection.

The adjacent simple returns are `0.1`, `-0.1`, and `1/11`. For a window of three observations the valid SMA values are `103` and `317/3`. The first return and first two SMA output cells must be empty.

Supply only one asset, currency, and consistent price basis per file. With real data, record its source, currency, date range, and raw/adjusted price convention. The program does not itself adjust splits or dividends.
