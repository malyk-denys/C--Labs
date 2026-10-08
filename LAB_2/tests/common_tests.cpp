#include "../src/common.hpp"
#include <limits>

int main() {
    auto require = [](bool ok) { if (!ok) throw std::runtime_error("CHECK FAILED"); };
    for (const auto value : {-19, -1, 0, 1, 4, 9, 25, 49, 121, 561}) require(!lab::is_prime(value));
    for (const auto value : {2, 3, 5, 17, 97, 541, 7919}) require(lab::is_prime(value));
    const std::uint64_t expected[] = {2,3,5,7,11,13,17,19,23,29};
    for (std::uint32_t n = 1; n <= 10; ++n) require(lab::nth_prime(n) == expected[n-1]);
    require(lab::nth_prime(100) == 541);
    require(lab::nth_prime(1000) == 7919);
    require(lab::nth_prime(1000000) == 15485863);
    for (const std::uint32_t n : {0u, 10000001u}) {
        bool caught = false;
        try { (void)lab::nth_prime(n); } catch (const std::out_of_range&) { caught = true; }
        require(caught);
    }
    std::int64_t value = 0;
    require(lab::parse_integer("+17", value) && value == 17);
    require(lab::parse_integer("-9223372036854775808", value) && value == std::numeric_limits<std::int64_t>::min());
    for (const auto* text : {"", "+", "+-1", "12abc", "1.5", "9223372036854775808"}) require(!lab::parse_integer(text, value));
    std::cout << "PASS: primality, sieve, range checks, strict parsing\n";
}
