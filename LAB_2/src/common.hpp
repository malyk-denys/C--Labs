#pragma once

#include <charconv>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

namespace lab {
inline void init_console() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

// from_chars перевіряє весь токен: "12abc" не є числом.
inline bool parse_integer(const std::string& token, std::int64_t& value) {
    if (token.empty()) return false;
    const char* begin = token.data();
    if (*begin == '+') {
        ++begin;
        if (begin == token.data() + token.size() || *begin < '0' || *begin > '9')
            return false;
    }
    const auto result = std::from_chars(begin, token.data() + token.size(), value);
    return result.ec == std::errc{} && result.ptr == token.data() + token.size();
}

inline bool is_prime(std::int64_t value) {
    if (value < 2) return false;
    if (value % 2 == 0) return value == 2;
    // Ділення замість d*d усуває переповнення.
    for (std::int64_t d = 3; d <= value / d; d += 2)
        if (value % d == 0) return false;
    return true;
}

inline bool read_index(std::uint32_t& n, std::uint32_t limit = 10000000) {
    std::string token;
    while (std::cin >> token) {
        if (token == "q" || token == "Q") return false;
        std::int64_t value = 0;
        if (parse_integer(token, value) && value >= 1 && value <= limit) {
            n = static_cast<std::uint32_t>(value);
            return true;
        }
        std::cout << "Некоректний номер. Введіть 1.." << limit << " або q: " << std::flush;
    }
    return false; // EOF також означає завершення вводу.
}

// Решето Ератосфена: O(B log log B) часу, O(B) бітів пам'яті.
// Локальний масив дозволяє одночасні виклики без спільних змінних.
inline std::uint64_t nth_prime(std::uint32_t n) {
    if (n == 0 || n > 10000000) throw std::out_of_range("n must be in 1..10000000");
    const double x = n;
    std::size_t bound = n < 6 ? 15 : static_cast<std::size_t>(x * (std::log(x) + std::log(std::log(x)))) + 16;
    for (;;) {
        std::vector<bool> prime(bound + 1, true);
        prime[0] = prime[1] = false;
        for (std::size_t p = 2; p <= bound / p; ++p)
            if (prime[p])
                for (std::size_t multiple = p * p; multiple <= bound; multiple += p)
                    prime[multiple] = false;
        std::uint32_t count = 0;
        for (std::size_t p = 2; p <= bound; ++p)
            if (prime[p] && ++count == n) return p;
        bound *= 2; // Запасний шлях, якщо початкова межа недостатня.
    }
}
} // namespace lab
