#include "common.hpp"
#include <chrono>
#include <future>
#include <iomanip>
#include <thread>

using Clock = std::chrono::steady_clock;
double milliseconds(Clock::duration time) {
    return std::chrono::duration<double, std::milli>(time).count();
}

void experiment(std::launch policy, std::uint32_t n, bool benchmark) {
    const char* name = policy == std::launch::deferred ? "deferred" : "async";
    std::cout << "\nРежим " << name << '\n';
    const auto start = Clock::now();
    auto result = std::async(policy, lab::nth_prime, n);
    if (benchmark) {
        // Однакова і явно позначена імітація роботи користувача в обох режимах.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "sqrt(n)=" << std::sqrt(static_cast<double>(n)) << '\n';
    } else {
        std::cout << "Функція від n: 1=sqrt, 2=sin (радіани), 3=ln, 0=пропустити: " << std::flush;
        std::string choice;
        while (std::cin >> choice) {
            if (choice == "0" || choice == "q") break;
            if (choice == "1") std::cout << "sqrt(n)=" << std::sqrt(static_cast<double>(n)) << '\n';
            else if (choice == "2") std::cout << "sin(n)=" << std::sin(static_cast<double>(n)) << '\n';
            else if (choice == "3") std::cout << "ln(n)=" << std::log(static_cast<double>(n)) << '\n';
            else { std::cout << "Введіть 0, 1, 2 або 3: " << std::flush; continue; }
            break;
        }
    }
    const auto before_get = Clock::now();
    const auto prime = result.get();
    const auto finish = Clock::now();
    std::cout << "prime(" << n << ")=" << prime << '\n'
              << std::fixed << std::setprecision(3)
              << "RESULT mode=" << name << " n=" << n << " prime=" << prime
              << " wait_ms=" << milliseconds(finish - before_get)
              << " total_ms=" << milliseconds(finish - start) << '\n';
}

int main(int argc, char* argv[]) {
    lab::init_console();
    try {
        const bool benchmark = argc > 1 && std::string(argv[1]) == "--benchmark";
        std::uint32_t n = 0;
        if (benchmark) {
            std::int64_t value = 1000000;
            if (argc > 3 || (argc == 3 && !lab::parse_integer(argv[2], value)) || value < 1 || value > 10000000)
                throw std::invalid_argument("Usage: task_5 --benchmark [n: 1..10000000]");
            n = static_cast<std::uint32_t>(value);
            std::cout << "Benchmark: 500 ms simulated user work per mode\n";
        } else {
            if (argc != 1) throw std::invalid_argument("Usage: task_5 [--benchmark [n]]");
            std::cout << "Номер простого числа (1..10000000 або q): " << std::flush;
            if (!lab::read_index(n)) return 0;
        }
        experiment(std::launch::deferred, n, benchmark);
        experiment(std::launch::async, n, benchmark);
    } catch (const std::exception& error) {
        std::cerr << "Помилка: " << error.what() << '\n';
        return 1;
    }
}
