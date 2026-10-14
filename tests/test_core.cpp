#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "course/core.hpp"

#include <cmath>
#include <limits>
#include <locale>
#include <random>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

std::vector<course::PriceRecord> parse(const std::string& text) {
    std::istringstream input(text);
    return course::read_prices_csv(input);
}

void check_csv_error(const std::string& text, std::size_t line = 0) {
    try {
        const auto records = parse(text);
        (void)records;
        FAIL_CHECK("invalid CSV was accepted");
    } catch (const std::runtime_error& error) {
        const std::string pattern = line == 0
            ? "[Ll]ine[ :]+[0-9]+"
            : "[Ll]ine[ :]+" + std::to_string(line) + "([^0-9]|$)";
        CHECK(std::regex_search(error.what(), std::regex(pattern)));
    } catch (...) {
        FAIL_CHECK("CSV errors must use std::runtime_error");
    }
}

// Relative tolerance 1e-12, with an absolute floor of 1e-12 near zero.
void check_values(const std::vector<double>& actual,
                  const std::vector<double>& expected,
                  double tolerance = 1e-12) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) {
        CAPTURE(i);
        CHECK(actual[i] == doctest::Approx(expected[i]).epsilon(tolerance).scale(1.0));
    }
}

std::vector<double> reference_means(const std::vector<double>& prices,
                                    std::size_t window) {
    std::vector<double> result;
    for (std::size_t start = 0; start + window <= prices.size(); ++start) {
        long double sum = 0.0L;
        for (std::size_t offset = 0; offset < window; ++offset) {
            sum += static_cast<long double>(prices[start + offset]);
        }
        result.push_back(static_cast<double>(sum / static_cast<long double>(window)));
    }
    return result;
}

std::vector<std::string> fields(const std::string& line) {
    std::vector<std::string> result;
    std::size_t start = 0;
    for (;;) {
        const auto comma = line.find(',', start);
        result.push_back(line.substr(start, comma - start));
        if (comma == std::string::npos) {
            return result;
        }
        start = comma + 1;
    }
}

std::vector<std::vector<std::string>> output_rows(const std::string& text) {
    std::istringstream input(text);
    std::vector<std::vector<std::string>> rows;
    std::string line;
    while (std::getline(input, line)) {
        rows.push_back(fields(line));
    }
    return rows;
}

std::vector<course::PriceRecord> sample_records() {
    return parse("date,close\n2026-10-01,100\n2026-10-02,110\n"
                 "2026-10-03,99\n2026-10-04,108\n");
}

class DecimalComma : public std::numpunct<char> {
protected:
    char do_decimal_point() const override { return ','; }
};

}  // namespace

TEST_CASE("CSV reads a documented price series in its original order") {
    const auto records = sample_records();
    REQUIRE(records.size() == 4);
    CHECK(records[0].date == "2026-10-01");
    CHECK(records[3].date == "2026-10-04");
    check_values(course::closing_prices(records), {100.0, 110.0, 99.0, 108.0});
}

TEST_CASE("CSV accepts surrounding whitespace CRLF and an initial UTF-8 BOM") {
    const auto records = parse("\xEF\xBB\xBF date , close \r\n"
                               "\t2026-10-01 , 1e2 \t\r\n"
                               "2026-10-02, 110\r\n");
    REQUIRE(records.size() == 2);
    CHECK(records[0].date == "2026-10-01");
    CHECK(records[0].close == 100.0);
    CHECK(records[1].close == 110.0);
    CHECK(parse("date,close\n2026-10-01,100").size() == 1);
}

TEST_CASE("CSV requires exactly the supported header and two unquoted columns") {
    const std::vector<std::pair<std::string, std::size_t>> cases{
        {"", 0},
        {"date,close\n", 0},
        {"Date,close\n2026-10-01,100\n", 1},
        {"close,date\n100,2026-10-01\n", 1},
        {"date,close,volume\n2026-10-01,100,1\n", 1},
        {"\"date\",close\n2026-10-01,100\n", 1},
        {"date,close\n\"2026-10-01\",100\n", 2},
        {"date,close\n2026-10-01,\"100\"\n", 2},
        {"date,close\n2026-10-01,100,1\n", 2},
        {"date,close\n2026-10-01\n", 2},
        {"date,close\n2026-10-01,\n", 2},
        {"date,close\n,100\n", 2},
        {"date,close\n2026-10-01,100\n\n", 3},
        {"date,close\n \t\n", 2},
    };
    for (const auto& example : cases) {
        CAPTURE(example.first);
        check_csv_error(example.first, example.second);
    }
}

TEST_CASE("CSV validates Gregorian dates including century leap-year rules") {
    CHECK(parse("date,close\n0001-01-01,1\n").size() == 1);
    CHECK(parse("date,close\n9999-12-31,1\n").size() == 1);
    CHECK(parse("date,close\n2000-02-29,1\n").size() == 1);
    CHECK(parse("date,close\n2024-02-29,1\n").size() == 1);
    const std::vector<std::string> invalid_dates{
        "0000-01-01", "10000-01-01", "2026-1-01", "2026-01-1",
        "2026/01/01", "2026-00-01", "2026-13-01", "2026-01-00",
        "2026-04-31", "2026-02-29", "1900-02-29", "2100-02-29",
        "2026-01-01x", "202a-01-01",
    };
    for (const auto& date : invalid_dates) {
        CAPTURE(date);
        check_csv_error("date,close\n" + date + ",100\n", 2);
    }
}

TEST_CASE("CSV rejects duplicate and decreasing dates instead of sorting") {
    check_csv_error("date,close\n2026-10-01,100\n2026-10-01,110\n", 3);
    check_csv_error("date,close\n2026-10-02,100\n2026-10-01,110\n", 3);
}

TEST_CASE("CSV rejects invalid nonpositive and nonfinite price fields") {
    for (const auto& price : {"0", "-1", "nan", "NaN", "inf", "-inf",
                              "1e9999", "100oops", "1 00"}) {
        CAPTURE(price);
        check_csv_error(std::string("date,close\n2026-10-01,") + price + "\n", 2);
    }
}

TEST_CASE("CSV reports an input stream failure as a runtime error") {
    std::istringstream input("date,close\n2026-10-01,100\n");
    input.setstate(std::ios::badbit);
    CHECK_THROWS_AS(course::read_prices_csv(input), std::runtime_error);
}

TEST_CASE("all indicators enforce finite positive nonempty input") {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    const std::vector<std::vector<double>> invalid{
        {}, {0.0}, {-1.0}, {nan}, {inf}, {-inf}, {1.0, 0.0}, {1.0, nan},
    };
    for (const auto& prices : invalid) {
        CHECK_THROWS_AS(course::validate_prices(prices), std::invalid_argument);
        CHECK_THROWS_AS(course::simple_returns(prices), std::invalid_argument);
        CHECK_THROWS_AS(course::sma_naive(prices, 1), std::invalid_argument);
        CHECK_THROWS_AS(course::sma_rolling(prices, 1), std::invalid_argument);
    }
    CHECK_NOTHROW(course::validate_prices({1.0, 2.0}));
}

TEST_CASE("simple returns agree with independently calculated examples") {
    check_values(course::simple_returns({100.0, 110.0, 99.0, 108.0}),
                 {0.1, -0.1, 1.0 / 11.0});
    CHECK(course::simple_returns({100.0}).empty());
    check_values(course::simple_returns({7.0, 7.0, 7.0}), {0.0, 0.0});
}

TEST_CASE("SMA algorithms agree with hand calculations and boundary windows") {
    const std::vector<double> prices{100.0, 110.0, 99.0, 108.0};
    for (const auto calculate : {course::sma_naive, course::sma_rolling}) {
        check_values(calculate(prices, 3), {103.0, 317.0 / 3.0});
        check_values(calculate(prices, 1), prices);
        check_values(calculate(prices, 4), {104.25});
        check_values(calculate({7.0}, 1), {7.0});
        CHECK_THROWS_AS(calculate(prices, 0), std::invalid_argument);
        CHECK_THROWS_AS(calculate(prices, 5), std::invalid_argument);
        CHECK_THROWS_AS(calculate(prices, std::numeric_limits<std::size_t>::max()),
                        std::invalid_argument);
    }
}

TEST_CASE("nonfinite return and SMA arithmetic is rejected") {
    const double smallest = std::numeric_limits<double>::min();
    const double largest = std::numeric_limits<double>::max();
    CHECK_THROWS_AS(course::simple_returns({smallest, largest}), std::overflow_error);
    CHECK_THROWS_AS(course::sma_naive({largest, largest}, 2), std::overflow_error);
    CHECK_THROWS_AS(course::sma_rolling({largest, largest}, 2), std::overflow_error);
}

TEST_CASE("compensated rolling updates preserve small values after a large price leaves") {
    const std::vector<double> prices{1e16, 1.0, 1.0, 1.0};
    const std::vector<double> expected{5e15, 1.0, 1.0};
    check_values(course::sma_naive(prices, 2), expected);
    check_values(course::sma_rolling(prices, 2), expected);
}

TEST_CASE("multiple deterministic workloads match a separately accumulated reference") {
    std::mt19937 generator(20261014);
    std::uniform_real_distribution<double> price_distribution(1.0, 10000.0);
    for (const std::size_t size : {1U, 2U, 8U, 31U, 128U, 257U, 1024U}) {
        std::vector<double> prices(size);
        for (auto& price : prices) {
            price = price_distribution(generator);
        }
        const std::vector<std::size_t> windows{
            1, size, size < 3 ? size : 3, size / 2 == 0 ? 1 : size / 2};
        for (const auto window : windows) {
            CAPTURE(size);
            CAPTURE(window);
            const auto expected = reference_means(prices, window);
            // This exercises accumulated updates; it is not a universal error bound.
            check_values(course::sma_naive(prices, window), expected, 1e-11);
            check_values(course::sma_rolling(prices, window), expected, 1e-11);
        }
    }
}

TEST_CASE("constant prices scaling and trailing causality have expected behavior") {
    const std::vector<double> constant(40, 12.5);
    check_values(course::sma_naive(constant, 7), std::vector<double>(34, 12.5));
    check_values(course::sma_rolling(constant, 7), std::vector<double>(34, 12.5));

    const std::vector<double> prices{3.0, 5.0, 4.0, 9.0, 12.0};
    auto scaled = prices;
    for (auto& price : scaled) {
        price *= 7.0;
    }
    check_values(course::simple_returns(scaled), course::simple_returns(prices));
    for (const auto calculate : {course::sma_naive, course::sma_rolling}) {
        auto expected = calculate(prices, 3);
        for (auto& mean : expected) {
            mean *= 7.0;
        }
        check_values(calculate(scaled, 3), expected);

        auto extended = prices;
        extended.push_back(1000.0);
        auto earlier_means = calculate(extended, 3);
        earlier_means.pop_back();
        check_values(earlier_means, calculate(prices, 3));
    }
    auto extended = prices;
    extended.push_back(1000.0);
    auto earlier_returns = course::simple_returns(extended);
    earlier_returns.pop_back();
    check_values(earlier_returns, course::simple_returns(prices));
}

TEST_CASE("CSV output aligns returns and SMA without filling unavailable values") {
    const auto records = sample_records();
    const auto prices = course::closing_prices(records);
    std::ostringstream output;
    course::write_analysis_csv(output, records, course::simple_returns(prices),
                               course::sma_rolling(prices, 3), 3);
    const auto rows = output_rows(output.str());
    REQUIRE(rows.size() == 5);
    CHECK(rows[0] == std::vector<std::string>{"date", "close", "simple_return", "sma"});
    for (std::size_t i = 1; i < rows.size(); ++i) {
        REQUIRE(rows[i].size() == 4);
        CHECK(rows[i][0] == records[i - 1].date);
        CHECK(std::stod(rows[i][1]) == records[i - 1].close);
    }
    CHECK(rows[1][2].empty());
    CHECK(rows[1][3].empty());
    CHECK(rows[2][3].empty());
    CHECK(std::stod(rows[2][2]) == doctest::Approx(0.1));
    CHECK(std::stod(rows[3][2]) == doctest::Approx(-0.1));
    CHECK(std::stod(rows[4][2]) == doctest::Approx(1.0 / 11.0));
    CHECK(std::stod(rows[3][3]) == doctest::Approx(103.0));
    CHECK(std::stod(rows[4][3]) == doctest::Approx(317.0 / 3.0));
}

TEST_CASE("CSV output round-trips doubles and uses a decimal point") {
    const double price = std::nextafter(100.0, 101.0);
    std::ostringstream output;
    output.imbue(std::locale(std::locale::classic(), new DecimalComma));
    course::write_analysis_csv(output, {{"2026-10-01", price}}, {}, {price}, 1);
    const auto rows = output_rows(output.str());
    REQUIRE(rows.size() == 2);
    REQUIRE(rows[1].size() == 4);
    CHECK(rows[1][2].empty());
    CHECK(std::stod(rows[1][1]) == price);
    CHECK(std::stod(rows[1][3]) == price);
}

TEST_CASE("CSV output rejects incompatible dimensions and nonfinite values") {
    const auto records = sample_records();
    const std::vector<double> returns{0.1, -0.1, 1.0 / 11.0};
    const std::vector<double> sma{103.0, 317.0 / 3.0};
    std::ostringstream output;
    CHECK_THROWS_AS(course::write_analysis_csv(output, records, {}, sma, 3),
                    std::invalid_argument);
    CHECK_THROWS_AS(course::write_analysis_csv(output, records, returns, {}, 3),
                    std::invalid_argument);
    CHECK_THROWS_AS(course::write_analysis_csv(output, records, returns, sma, 0),
                    std::invalid_argument);
    CHECK_THROWS_AS(course::write_analysis_csv(output, records, returns, sma, 5),
                    std::invalid_argument);
    CHECK_THROWS_AS(course::write_analysis_csv(output, {}, {}, {}, 1),
                    std::invalid_argument);
    const double inf = std::numeric_limits<double>::infinity();
    CHECK_THROWS_AS(course::write_analysis_csv(output, records, {inf, 0.0, 0.0}, sma, 3),
                    std::invalid_argument);
    CHECK_THROWS_AS(course::write_analysis_csv(output, records, returns, {inf, 1.0}, 3),
                    std::invalid_argument);
    auto invalid_records = records;
    invalid_records[0].close = inf;
    CHECK_THROWS_AS(course::write_analysis_csv(output, invalid_records, returns, sma, 3),
                    std::invalid_argument);
}

TEST_CASE("CSV output reports write failures") {
    std::ostringstream output;
    output.setstate(std::ios::badbit);
    CHECK_THROWS_AS(course::write_analysis_csv(output, {{"2026-10-01", 100.0}},
                                              {}, {100.0}, 1),
                    std::runtime_error);
}

TEST_CASE("CSV preserves representable positive subnormal prices") {
    const auto records = parse("date,close\n2026-10-01,1e-310\n"
                               "2026-10-02,+1e-320\n"
                               "2026-10-03,4.9406564584124654e-324\n");
    const std::vector<double> expected{
        1e-310, 1e-320, std::numeric_limits<double>::denorm_min()};
    REQUIRE(records.size() == expected.size());
    for (std::size_t i = 0; i < records.size(); ++i) {
        CAPTURE(i);
        CHECK(records[i].close > 0.0);
        // An absolute Approx tolerance could incorrectly accept zero here.
        CHECK(records[i].close == expected[i]);
    }
}

TEST_CASE("CSV rejects out-of-range numbers and trailing text near subnormal range") {
    for (const auto& price : {"1e-999", "1e999", "1e-310tail", "+1e-320tail"}) {
        CAPTURE(price);
        check_csv_error(std::string("date,close\n2026-10-01,") + price + "\n", 2);
    }
}

TEST_CASE("SMA retains the final small window after much larger prices leave") {
    const std::vector<std::pair<std::vector<double>, double>> cases{
        {{1e308, 1e200, 1.0, 1.0, 1.0}, 1.0},
        {{1e16, 1.0, 1e-16, 1e-16, 1e-16}, 1e-16},
    };
    for (const auto& example : cases) {
        for (const auto calculate : {course::sma_naive, course::sma_rolling}) {
            const auto means = calculate(example.first, 3);
            REQUIRE(means.size() == 3);
            const double actual = means.back();
            const double expected = example.second;
            CAPTURE(actual);
            CAPTURE(expected);
            CHECK(actual > 0.0);
            // Pure relative tolerance: a zero or badly cancelled answer fails.
            CHECK(std::abs(actual - expected) <= 1e-12 * std::abs(expected));
        }
    }
}
