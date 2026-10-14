#include "course/core.hpp"

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace course {
namespace {

void require_finite(double value) {
    if (!std::isfinite(value)) {
        throw std::overflow_error("nonfinite arithmetic in SMA calculation");
    }
}

void validate_window(std::size_t size, std::size_t window) {
    if (window == 0 || window > size) {
        throw std::invalid_argument("SMA window must be between 1 and the price count");
    }
}

// Neumaier summation keeps a correction for rounding lost in each addition.
// A sum can be merged by adding both its main value and its correction.
// This reduces rounding errors but does not provide exact arithmetic.
class CompensatedSum {
public:
    void add(double value) {
        const double next_sum = sum_ + value;
        require_finite(next_sum);

        double residual;
        double rounding_error;
        if (std::abs(sum_) >= std::abs(value)) {
            residual = sum_ - next_sum;
            require_finite(residual);
            rounding_error = residual + value;
        } else {
            residual = value - next_sum;
            require_finite(residual);
            rounding_error = residual + sum_;
        }
        require_finite(rounding_error);

        const double next_correction = correction_ + rounding_error;
        require_finite(next_correction);
        sum_ = next_sum;
        correction_ = next_correction;
    }

    void merge(const CompensatedSum& other) {
        add(other.sum_);
        add(other.correction_);
    }

    double mean(std::size_t count) const {
        const double total = sum_ + correction_;
        require_finite(total);
        const double result = total / static_cast<double>(count);
        require_finite(result);
        if (result <= 0.0) {
            throw std::overflow_error("nonpositive result in SMA calculation");
        }
        return result;
    }

private:
    double sum_ = 0.0;
    double correction_ = 0.0;
};

CompensatedSum sum_window(const std::vector<double>& prices,
                          std::size_t first,
                          std::size_t window) {
    CompensatedSum sum;
    for (std::size_t offset = 0; offset < window; ++offset) {
        sum.add(prices[first + offset]);
    }
    return sum;
}

// Each stack entry stores the aggregate of that stack's prefix. Removing an
// entry reveals a previously saved sum, instead of subtracting a large price
// from a rounded total and potentially losing the remaining small prices.
class SlidingSumQueue {
public:
    explicit SlidingSumQueue(std::size_t capacity) {
        front_.reserve(capacity);
        back_.reserve(capacity);
    }

    void push(double price) {
        append(back_, price);
    }

    void pop() {
        if (front_.empty()) {
            while (!back_.empty()) {
                append(front_, back_.back().price);
                back_.pop_back();
            }
        }
        front_.pop_back();
    }

    double mean(std::size_t count) const {
        if (front_.empty()) {
            return back_.back().aggregate.mean(count);
        }
        CompensatedSum total = front_.back().aggregate;
        if (!back_.empty()) {
            total.merge(back_.back().aggregate);
        }
        return total.mean(count);
    }

private:
    struct Entry {
        double price;
        CompensatedSum aggregate;
    };

    static void append(std::vector<Entry>& stack, double price) {
        CompensatedSum aggregate;
        if (!stack.empty()) {
            aggregate = stack.back().aggregate;
        }
        aggregate.add(price);
        stack.push_back({price, aggregate});
    }

    std::vector<Entry> front_;
    std::vector<Entry> back_;
};

}  // namespace

void validate_prices(const std::vector<double>& prices) {
    if (prices.empty()) {
        throw std::invalid_argument("at least one price is required");
    }
    for (std::size_t i = 0; i < prices.size(); ++i) {
        if (!std::isfinite(prices[i]) || prices[i] <= 0.0) {
            throw std::invalid_argument("price at index " + std::to_string(i)
                                        + " must be finite and strictly positive");
        }
    }
}

std::vector<double> simple_returns(const std::vector<double>& prices) {
    validate_prices(prices);
    std::vector<double> returns;
    returns.reserve(prices.size() - 1);

    for (std::size_t i = 1; i < prices.size(); ++i) {
        const double result = prices[i] / prices[i - 1] - 1.0;
        if (!std::isfinite(result)) {
            throw std::overflow_error("nonfinite simple return at index "
                                      + std::to_string(i - 1));
        }
        returns.push_back(result);
    }
    return returns;
}

std::vector<double> sma_naive(const std::vector<double>& prices, std::size_t window) {
    validate_prices(prices);
    validate_window(prices.size(), window);
    if (window == 1) {
        return prices;
    }

    const std::size_t output_count = prices.size() - window + 1;
    std::vector<double> means;
    means.reserve(output_count);
    for (std::size_t first = 0; first < output_count; ++first) {
        means.push_back(sum_window(prices, first, window).mean(window));
    }
    return means;
}

std::vector<double> sma_rolling(const std::vector<double>& prices, std::size_t window) {
    validate_prices(prices);
    validate_window(prices.size(), window);
    if (window == 1) {
        return prices;
    }

    std::vector<double> means;
    means.reserve(prices.size() - window + 1);
    SlidingSumQueue queue(window);
    for (std::size_t i = 0; i < window; ++i) {
        queue.push(prices[i]);
    }
    means.push_back(queue.mean(window));

    for (std::size_t incoming = window; incoming < prices.size(); ++incoming) {
        // Each price is pushed, transferred between stacks, and removed at
        // most once: O(n) total work, with O(window) saved aggregate states.
        queue.pop();
        queue.push(prices[incoming]);
        means.push_back(queue.mean(window));
    }
    return means;
}

}  // namespace course
