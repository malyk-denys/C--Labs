#include "common.hpp"
#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <thread>

// Усі дані передаються аргументами. Глобальних змінних немає.
void PrimeWorker(std::uint32_t n, std::promise<std::uint64_t> first,
                 std::promise<bool> signal, std::promise<std::uint64_t> tenth,
                 std::promise<void> finished) {
    try {
        first.set_value(lab::nth_prime(n));
        signal.set_value(true);
        tenth.set_value(lab::nth_prime(n * 10));
        finished.set_value_at_thread_exit();
    } catch (...) {
        // Незаповнені promises також повідомлять broken_promise після знищення.
        finished.set_exception_at_thread_exit(std::current_exception());
    }
}

void SqrtWorker(std::uint32_t n, std::future<bool> signal,
                std::shared_ptr<std::mutex> output_mutex, std::promise<void> finished) {
    try {
        if (signal.get()) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            std::lock_guard<std::mutex> lock(*output_mutex);
            std::cout << "sqrt(" << n << ")=" << std::sqrt(static_cast<double>(n)) << '\n';
        }
        finished.set_value_at_thread_exit();
    } catch (...) {
        finished.set_exception_at_thread_exit(std::current_exception());
    }
}

int main() {
    lab::init_console();
    std::cout << "n (1..1000000 або q; також обчислюється prime(10*n)): " << std::flush;
    std::uint32_t n = 0;
    if (!lab::read_index(n, 1000000)) return 0;
    std::promise<std::uint64_t> first, tenth;
    std::promise<bool> signal;
    std::promise<void> prime_done, sqrt_done;
    auto first_result = first.get_future();
    auto tenth_result = tenth.get_future();
    auto start_sqrt = signal.get_future();
    auto prime_completion = prime_done.get_future();
    auto sqrt_completion = sqrt_done.get_future();
    auto output_mutex = std::make_shared<std::mutex>();
    std::thread prime_thread(PrimeWorker, n, std::move(first), std::move(signal),
                             std::move(tenth), std::move(prime_done));
    prime_thread.detach();
    std::thread sqrt_thread(SqrtWorker, n, std::move(start_sqrt), output_mutex, std::move(sqrt_done));
    sqrt_thread.detach();
    int status = 0;
    try {
        const auto p1 = first_result.get();
        {
            std::lock_guard<std::mutex> lock(*output_mutex);
            std::cout << "\nprime(" << n << ")=" << p1 << '\n';
        }
        const auto p10 = tenth_result.get();
        {
            std::lock_guard<std::mutex> lock(*output_mutex);
            std::cout << "prime(" << n * 10 << ")=" << p10 << '\n';
        }
    } catch (const std::exception& error) {
        std::lock_guard<std::mutex> lock(*output_mutex);
        std::cerr << "Помилка результату: " << error.what() << '\n';
        status = 1;
    }
    // Очікуємо ОБИДВА потоки навіть у разі помилки одного з них.
    for (auto* completion : {&prime_completion, &sqrt_completion}) {
        try { completion->get(); }
        catch (const std::exception& error) {
            std::lock_guard<std::mutex> lock(*output_mutex);
            std::cerr << "Помилка потоку: " << error.what() << '\n';
            status = 1;
        }
    }
    return status;
}
