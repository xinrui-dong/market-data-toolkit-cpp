#include "course/core.hpp"

#include <charconv>
#include <cmath>
#include <iomanip>
#include <istream>
#include <limits>
#include <locale>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace course {
namespace {

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(" \t\r");
    return text.substr(first, last - first + 1);
}

[[noreturn]] void csv_error(std::size_t line, const std::string& message) {
    throw std::runtime_error("CSV line " + std::to_string(line) + ": " + message);
}

std::pair<std::string, std::string> split_record(const std::string& line,
                                               std::size_t number) {
    if (line.find('"') != std::string::npos) {
        csv_error(number, "quoted fields are outside the supported CSV format");
    }
    const auto comma = line.find(',');
    if (comma == std::string::npos || line.find(',', comma + 1) != std::string::npos) {
        csv_error(number, "expected exactly two columns: date,close");
    }
    return {trim(line.substr(0, comma)), trim(line.substr(comma + 1))};
}

bool valid_date(const std::string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') {
        return false;
    }
    for (std::size_t i = 0; i < date.size(); ++i) {
        if (i != 4 && i != 7 && (date[i] < '0' || date[i] > '9')) {
            return false;
        }
    }
    const int year = std::stoi(date.substr(0, 4));
    const int month = std::stoi(date.substr(5, 2));
    const int day = std::stoi(date.substr(8, 2));
    if (year == 0 || month < 1 || month > 12 || day < 1) {
        return false;
    }
    const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    const int maximum = days[month - 1] + (month == 2 && leap ? 1 : 0);
    return day <= maximum;
}

double parse_close(const std::string& field, std::size_t line) {
    if (field.empty()) {
        csv_error(line, "close is missing");
    }
    const char* first = field.data();
    const char* last = first + field.size();
    // Floating from_chars is locale independent and accepts representable
    // subnormal values. Its grammar omits a leading '+', which we allow here.
    if (*first == '+') {
        ++first;
    }
    double value = 0.0;
    const auto parsed = std::from_chars(first, last, value, std::chars_format::general);
    if (parsed.ec != std::errc{} || parsed.ptr != last) {
        csv_error(line, "close must be a complete decimal number in double range");
    }
    if (!std::isfinite(value) || value <= 0.0) {
        csv_error(line, "close must be finite and strictly positive");
    }
    return value;
}

}  // namespace

std::vector<PriceRecord> read_prices_csv(std::istream& input) {
    std::string line;
    if (!std::getline(input, line)) {
        csv_error(1, "expected the date,close header");
    }
    if (line.compare(0, 3, "\xEF\xBB\xBF") == 0) {
        line.erase(0, 3);
    }
    const auto header = split_record(line, 1);
    if (header.first != "date" || header.second != "close") {
        csv_error(1, "header must be date,close (case-sensitive)");
    }

    std::vector<PriceRecord> records;
    std::size_t number = 1;
    while (std::getline(input, line)) {
        ++number;
        if (trim(line).empty()) {
            csv_error(number, "blank records are not supported");
        }
        auto fields = split_record(line, number);
        if (!valid_date(fields.first)) {
            csv_error(number, "date must be a valid Gregorian YYYY-MM-DD (year 0001-9999)");
        }
        if (!records.empty() && fields.first <= records.back().date) {
            csv_error(number, "dates must be strictly increasing; duplicates are not allowed");
        }
        const double close = parse_close(fields.second, number);
        records.push_back({std::move(fields.first), close});
    }
    if (input.bad() || (input.fail() && !input.eof())) {
        csv_error(number + 1, "could not read the input stream");
    }
    if (records.empty()) {
        csv_error(2, "at least one price record is required");
    }
    return records;
}

std::vector<double> closing_prices(const std::vector<PriceRecord>& records) {
    std::vector<double> prices;
    prices.reserve(records.size());
    for (const auto& record : records) {
        prices.push_back(record.close);
    }
    return prices;
}

void write_analysis_csv(std::ostream& output,
                        const std::vector<PriceRecord>& records,
                        const std::vector<double>& returns,
                        const std::vector<double>& sma,
                        std::size_t window) {
    const auto n = records.size();
    if (n == 0 || window == 0 || window > n ||
        returns.size() != n - 1 || sma.size() != n - window + 1) {
        throw std::invalid_argument("CSV output sizes or SMA window are inconsistent");
    }
    validate_prices(closing_prices(records));
    for (std::size_t i = 0; i < n; ++i) {
        if (!valid_date(records[i].date) ||
            (i > 0 && records[i].date <= records[i - 1].date)) {
            throw std::invalid_argument("CSV output dates must be valid and strictly increasing");
        }
    }
    for (double value : returns) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("CSV output returns must be finite");
        }
    }
    for (double value : sma) {
        if (!std::isfinite(value) || value <= 0.0) {
            throw std::invalid_argument("CSV output SMA values must be finite and positive");
        }
    }

    // Format separately so the caller's locale and precision are unchanged.
    std::ostringstream csv;
    csv.imbue(std::locale::classic());
    csv << std::setprecision(std::numeric_limits<double>::max_digits10);
    csv << "date,close,simple_return,sma\n";
    for (std::size_t i = 0; i < n; ++i) {
        csv << records[i].date << ',' << records[i].close << ',';
        if (i > 0) {
            csv << returns[i - 1];
        }
        csv << ',';
        if (i >= window - 1) {
            csv << sma[i - window + 1];
        }
        csv << '\n';
    }
    output << csv.str();
    output.flush();
    if (!output) {
        throw std::runtime_error("could not write the analysis CSV");
    }
}

}  // namespace course
