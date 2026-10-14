#include "course/core.hpp"

#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace {

struct Options {
    std::string input;
    std::string output;
    std::string algorithm = "rolling";
    std::size_t window = 0;
};

void usage(std::ostream& output) {
    output << "Usage: market_data --input FILE --window N "
              "[--output FILE] [--algorithm naive|rolling]\n"
              "       market_data --help\n"
              "Input: date,close CSV for one finite positive-price series.\n"
              "The window counts observations. Default output is stdout.\n";
}

Options parse_options(int argc, char* argv[]) {
    Options options;
    bool seen_input = false;
    bool seen_output = false;
    bool seen_window = false;
    bool seen_algorithm = false;
    for (int i = 1; i < argc; ++i) {
        const std::string flag = argv[i];
        if (flag != "--input" && flag != "--output" &&
            flag != "--window" && flag != "--algorithm") {
            throw std::invalid_argument("unknown option: " + flag);
        }
        if (i + 1 >= argc) {
            throw std::invalid_argument("missing value for " + flag);
        }
        const std::string value = argv[++i];
        bool* seen = flag == "--input" ? &seen_input :
                     flag == "--output" ? &seen_output :
                     flag == "--window" ? &seen_window : &seen_algorithm;
        if (*seen) {
            throw std::invalid_argument("duplicate option: " + flag);
        }
        *seen = true;
        if (value.empty() || value.rfind("--", 0) == 0) {
            throw std::invalid_argument("missing value for " + flag);
        }
        if (flag == "--input") {
            options.input = value;
        } else if (flag == "--output") {
            options.output = value;
        } else if (flag == "--algorithm") {
            if (value != "naive" && value != "rolling") {
                throw std::invalid_argument("algorithm must be naive or rolling");
            }
            options.algorithm = value;
        } else {
            const auto parsed = std::from_chars(value.data(), value.data() + value.size(),
                                                 options.window);
            if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() ||
                options.window == 0) {
                throw std::invalid_argument("window must be a positive integer in range");
            }
        }
    }
    if (!seen_input || !seen_window) {
        throw std::invalid_argument("--input and --window are required");
    }
    return options;
}

bool same_path(const std::string& first, const std::string& second) {
    namespace fs = std::filesystem;
    std::error_code error;
    if (fs::equivalent(first, second, error) && !error) {
        return true;
    }
    return fs::absolute(first).lexically_normal() == fs::absolute(second).lexically_normal();
}

}  // namespace

/** @brief Validate price CSV and calculate indicators. @return 0 on success. */
int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        usage(std::cout);
        return 0;
    }
    Options options;
    try {
        options = parse_options(argc, argv);
    } catch (const std::invalid_argument& error) {
        std::cerr << "Argument error: " << error.what() << '\n';
        usage(std::cerr);
        return 2;
    }
    try {
        if (!options.output.empty() && same_path(options.input, options.output)) {
            throw std::runtime_error("input and output must be different files");
        }
        std::ifstream input(options.input);
        if (!input) {
            throw std::runtime_error("could not open input file: " + options.input);
        }
        const auto records = course::read_prices_csv(input);
        const auto prices = course::closing_prices(records);
        const auto returns = course::simple_returns(prices);
        const auto sma = options.algorithm == "naive"
            ? course::sma_naive(prices, options.window)
            : course::sma_rolling(prices, options.window);

        // Calculate before opening an output file: bad input must not leave
        // an empty or apparently successful result file.
        if (options.output.empty()) {
            course::write_analysis_csv(std::cout, records, returns, sma, options.window);
        } else {
            std::ofstream output(options.output);
            if (!output) {
                throw std::runtime_error("could not open output file: " + options.output);
            }
            course::write_analysis_csv(output, records, returns, sma, options.window);
            output.close();
            if (!output) {
                throw std::runtime_error("could not close output file: " + options.output);
            }
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
