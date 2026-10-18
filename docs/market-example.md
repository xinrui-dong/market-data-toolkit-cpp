# Real-data example: EUR/USD reference rates

This example applies the same CSV validation, simple returns and two SMA
algorithms to **22 actual ECB observations from 2 to 31 January 2025**. It uses
a frozen local snapshot, so building and testing do not contact a data service.

## Source and price interpretation

Source: ECB statistics. Returns and SMA below are own calculations.

- Series: `EXR.D.USD.EUR.SP00.A`, daily US dollars per euro.
- Original data: [ECB API request](https://data-api.ecb.europa.eu/service/data/EXR/D.USD.EUR.SP00.A?startPeriod=2025-01-02&endPeriod=2025-01-31&format=csvdata).
- Retrieval date: 18 October 2026.
- `data/ecb_eurusd_2025_01_source.csv` preserves the downloaded CSV and metadata.
- `data/ecb_eurusd_2025_01.csv` selects `TIME_PERIOD` and `OBS_VALUE`, renaming
  them to `date` and `close`. Every date and rate token is unchanged, in source
  order. No missing calendar dates are filled.
- `data/ecb_eurusd_2025_01_provenance.json` records the source, interpretation,
  transformation and SHA-256 hashes of both files.

Here `close` is the program's existing numeric input column; the values are
**ECB reference rates, not exchange closing prices or executable quotes**.
ECB describes the rates as informational and discourages transaction use.
See the [reference-rate description](https://www.ecb.europa.eu/stats/policy_and_exchange_rates/euro_reference_exchange_rates/html/index.en.html)
and [reuse conditions](https://www.ecb.europa.eu/services/using-our-site/disclaimer/html/index.en.html).
The raw observations are reproduced without numerical changes. The returns
and SMA are explicitly identified as this project's calculations.

## Run and inspect

After building, run from the repository root on Linux or macOS:

~~~sh
./build/bin/market_data --input data/ecb_eurusd_2025_01.csv --window 5 --algorithm naive --output build/ecb_naive.csv
./build/bin/market_data --input data/ecb_eurusd_2025_01.csv --window 5 --algorithm rolling --output build/ecb_rolling.csv
python3 scripts/check_market_example.py --app build/bin/market_data --data-dir data
~~~

For the default Visual Studio Release build on Windows, use
`build/bin/market_data.exe` and `python` in those commands.
The complete PowerShell verification command is `./scripts/verify.ps1`.

The two CSV outputs preserve all 22 dates and rates. The first return is empty;
the first four SMA cells are empty because the window contains five observations.
The checker validates both algorithms against independent 50-digit Python
`Decimal` calculations: 42 returns and 36 complete SMA values, as well as source
hashes, unchanged normalization, output alignment and empty warm-up cells.
This check also runs in CTest and appears explicitly in the CI log.

Selected expected values, rounded here for reading:

| Date | USD per EUR | Adjacent rate change | SMA(5), USD per EUR |
|---|---:|---:|---:|
| 2025-01-02 | 1.0321 | empty | empty |
| 2025-01-03 | 1.0299 | -0.213158% | empty |
| 2025-01-08 | 1.0286 | -1.029539% | 1.03450 |
| 2025-01-09 | 1.0305 | +0.184717% | 1.03418 |
| 2025-01-31 | 1.0393 | -0.096126% | 1.04286 |

For the first complete window,
`(1.0321 + 1.0299 + 1.0426 + 1.0393 + 1.0286) / 5 = 1.0345`.
The tool writes return fractions; the table multiplies them by 100 for display.
Its CSV may show additional digits from binary floating-point arithmetic.

## What can be concluded

Between the first and last observations, the quoted USD value of one EUR
increased by `1.0393 / 1.0321 - 1`, approximately **0.697607%**. A positive
rate change in this quotation means EUR appreciation relative to USD. Reversing
the currency quotation changes the calculation; it is not just a label change.

On 31 January the rate is below its five-observation SMA. That describes its
position relative to recent reference rates; it does not predict a later rate.
The five-observation window can span more than five calendar days.

This is a reproducible historical data-processing example. Reference-rate
changes alone do not include interest, financing, spreads or trading costs,
and the example does not implement a trading strategy or measure its profit.
Synthetic data remains in the performance benchmark so both algorithms are
timed on identical deterministic workloads of controlled sizes.
