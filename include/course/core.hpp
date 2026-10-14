#pragma once

/** @file core.hpp @brief Historical price input, indicators, and CSV output. */
#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

/** @brief Data validation and indicators for a single positive-price series. */
namespace course {

/** @brief A dated close observation in one asset, currency, and price basis. */
struct PriceRecord {
    std::string date; ///< Gregorian date in YYYY-MM-DD format.
    double close;    ///< Finite, strictly positive price.
};

/**
 * @brief Read the restricted date,close CSV format and validate all records.
 * @param input Text stream; CRLF and surrounding spaces/tabs are accepted.
 * @return Nonempty observations with valid, strictly increasing dates.
 * @throws std::runtime_error On I/O errors or invalid input, with a line number.
 * @details Quoted fields, extra columns, blank records, duplicate dates, and
 * silent sorting or filling are not supported. An optional UTF-8 BOM is allowed.
 */
std::vector<PriceRecord> read_prices_csv(std::istream& input);

/**
 * @brief Extract the close values from observations.
 * @param records Observations, normally returned by read_prices_csv.
 * @return Close prices in the original order.
 */
std::vector<double> closing_prices(const std::vector<PriceRecord>& records);

/**
 * @brief Validate the numeric input contract shared by all indicators.
 * @param prices A nonempty sequence of finite, strictly positive values.
 * @throws std::invalid_argument When the contract is violated.
 */
void validate_prices(const std::vector<double>& prices);

/**
 * @brief Calculate adjacent-observation simple price returns.
 * @param prices A nonempty sequence satisfying validate_prices.
 * @return n-1 values, where element i is prices[i+1]/prices[i]-1.
 * @throws std::invalid_argument On invalid prices.
 * @throws std::overflow_error When a computed return is not finite.
 * @details A one-price input returns an empty vector. Dates need not be daily;
 * these are price changes, not automatically total returns including dividends.
 */
std::vector<double> simple_returns(const std::vector<double>& prices);

/**
 * @brief Calculate trailing SMA by recomputing each window.
 * @param prices Nonempty, finite, positive prices.
 * @param window Number of observations; 1 <= window <= prices.size().
 * @return n-window+1 means, starting at the first complete window.
 * @throws std::invalid_argument On invalid prices or window.
 * @throws std::overflow_error On nonfinite arithmetic or a nonpositive computed mean.
 * @details Uses compensated summation. Time is O((n-window+1)*window+n),
 * including input validation. Floating-point results are not exact arithmetic.
 */
std::vector<double> sma_naive(const std::vector<double>& prices, std::size_t window);

/**
 * @brief Calculate the same trailing SMA using a two-stack aggregate queue.
 * @param prices Nonempty, finite, positive prices.
 * @param window Number of observations; 1 <= window <= prices.size().
 * @return The same output alignment as sma_naive.
 * @throws std::invalid_argument On invalid prices or window.
 * @throws std::overflow_error On nonfinite arithmetic or a nonpositive computed mean.
 * @details Cached compensated sums avoid subtracting a large departing price
 * from the current sum. Each price is pushed, transferred, and removed at most
 * once: O(n) total work and O(window) extra workspace, besides the output.
 * This reduces cancellation errors without a universal accuracy guarantee.
 */
std::vector<double> sma_rolling(const std::vector<double>& prices, std::size_t window);

/**
 * @brief Write aligned date,close,simple_return,sma output.
 * @param output Destination text stream.
 * @param records Validated price observations.
 * @param returns n-1 returns from simple_returns.
 * @param sma n-window+1 values from either SMA algorithm.
 * @param window Valid SMA observation count.
 * @throws std::invalid_argument On incompatible sizes or nonfinite values.
 * @throws std::runtime_error On write errors.
 * @details The first return and first window-1 SMA cells are empty. Numeric
 * output uses a decimal point and enough digits to round-trip a double.
 */
void write_analysis_csv(std::ostream& output,
                        const std::vector<PriceRecord>& records,
                        const std::vector<double>& returns,
                        const std::vector<double>& sma,
                        std::size_t window);

}  // namespace course
