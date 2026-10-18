#!/usr/bin/env python3
"""Check the frozen ECB example and CLI results against Decimal arithmetic."""

import argparse
import csv
from decimal import Decimal, localcontext
import hashlib
import io
import json
import math
from pathlib import Path
import subprocess


def require(condition, message):
    if not condition:
        raise ValueError(message)


def check_example(app, data_dir):
    metadata = json.loads(
        (data_dir / "ecb_eurusd_2025_01_provenance.json").read_text(encoding="utf-8")
    )
    require(metadata["series"] == "EXR.D.USD.EUR.SP00.A", "unexpected ECB series")
    for name, expected in metadata["files"].items():
        content = (data_dir / name).read_bytes()
        require(len(content) == expected["bytes"], "snapshot size changed: " + name)
        require(hashlib.sha256(content).hexdigest() == expected["sha256"],
                "snapshot checksum changed: " + name)

    with (data_dir / "ecb_eurusd_2025_01_source.csv").open(
        encoding="utf-8-sig", newline=""
    ) as stream:
        original = list(csv.DictReader(stream))
    fixture = data_dir / "ecb_eurusd_2025_01.csv"
    with fixture.open(encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream)
        require(reader.fieldnames == ["date", "close"], "unexpected normalized header")
        records = list(reader)
    require(len(original) == len(records) == metadata["observations"] == 22,
            "expected all 22 source observations")
    for raw, row in zip(original, records):
        require(raw["KEY"] == "EXR.D.USD.EUR.SP00.A", "mixed source series")
        require(raw["OBS_STATUS"] == "A" and raw["UNIT"] == "USD"
                and raw["UNIT_MULT"] == "0", "unexpected source observation metadata")
        require((raw["TIME_PERIOD"], raw["OBS_VALUE"]) == (row["date"], row["close"]),
                "normalization changed an observation")
    require(records[0] == {"date": "2025-01-02", "close": "1.0321"},
            "unexpected first observation")
    require(records[-1] == {"date": "2025-01-31", "close": "1.0393"},
            "unexpected last observation")

    # Decimal uses the original published decimal tokens, independently of the
    # C++ summation implementation. The CLI must preserve warm-up empty cells.
    window = 5
    with localcontext() as context:
        context.prec = 50
        prices = [Decimal(row["close"]) for row in records]
        expected_returns = [None] + [prices[i] / prices[i - 1] - 1
                                    for i in range(1, len(prices))]
        expected_sma = [None] * (window - 1) + [
            sum(prices[i - window + 1:i + 1]) / window
            for i in range(window - 1, len(prices))
        ]
        require(expected_sma[4] == Decimal("1.0345"), "independent SMA check failed")
        for algorithm in ("naive", "rolling"):
            run = subprocess.run(
                [str(app), "--input", str(fixture), "--window", str(window),
                 "--algorithm", algorithm],
                capture_output=True, text=True, encoding="utf-8", timeout=20,
                check=False,
            )
            require(run.returncode == 0 and not run.stderr,
                    algorithm + " example failed: " + run.stderr)
            output = csv.DictReader(io.StringIO(run.stdout))
            require(output.fieldnames == ["date", "close", "simple_return", "sma"],
                    "unexpected output columns")
            rows = list(output)
            require(len(rows) == len(records), algorithm + " changed the row count")
            for i, row in enumerate(rows):
                require(row["date"] == records[i]["date"], "output date alignment changed")
                for column, expected in (("close", prices[i]),
                                         ("simple_return", expected_returns[i]),
                                         ("sma", expected_sma[i])):
                    value = row[column]
                    if expected is None:
                        require(value == "", "warm-up cell must be empty")
                    else:
                        actual = float(value)
                        require(math.isfinite(actual) and math.isclose(
                            actual, float(expected), rel_tol=5e-13, abs_tol=5e-15
                        ), "{} row {} {} differs from Decimal reference".format(
                            algorithm, i + 1, column))
        period_change = prices[-1] / prices[0] - 1

    return {"source": "ECB statistics", "series": metadata["series"],
            "observations": len(records), "window": window,
            "algorithms_checked": ["naive", "rolling"],
            "return_checks": 2 * (len(records) - 1),
            "sma_checks": 2 * (len(records) - window + 1),
            "first_sma": str(expected_sma[window - 1]),
            "last_sma": str(expected_sma[-1]),
            "start_to_end_rate_change": str(period_change), "status": "passed"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app", required=True, type=Path)
    parser.add_argument("--data-dir", required=True, type=Path)
    options = parser.parse_args()
    print(json.dumps(check_example(options.app.resolve(), options.data_dir.resolve()), indent=2))


if __name__ == "__main__":
    main()
