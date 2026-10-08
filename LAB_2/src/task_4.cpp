#include "common.hpp"
#include <condition_variable>
#include <future>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

// nullopt є маркером кінця; жодне ціле число не резервуємо.
std::queue<std::optional<std::int64_t>> data_queue;
std::mutex data_mutex;
std::condition_variable data_ready;

void DataPreparation(std::promise<void> finished) {
    std::cout << "Цілі числа через пробіл; q завершує введення:\n";
    std::queue<std::optional<std::int64_t>> input;
    std::string token;
    while (std::cin >> token && token != "q" && token != "Q") {
        std::int64_t value = 0;
        if (lab::parse_integer(token, value)) input.push(value);
        else std::cout << "Пропущено некоректне число: " << token << '\n';
    }
    input.push(std::nullopt);
    {
        std::lock_guard<std::mutex> lock(data_mutex);
        data_queue.swap(input); // Публікуємо весь пакет лише після завершення вводу.
    }
    data_ready.notify_one();
    finished.set_value_at_thread_exit();
}

void DataProcessing() {
    std::unique_lock<std::mutex> lock(data_mutex);
    data_ready.wait(lock, [] { return !data_queue.empty(); });
    std::queue<std::optional<std::int64_t>> numbers;
    numbers.swap(data_queue);
    lock.unlock();
    std::cout << "Прості числа:";
    while (!numbers.empty()) {
        const auto value = numbers.front();
        numbers.pop();
        if (!value) break;
        if (lab::is_prime(*value)) std::cout << ' ' << *value;
    }
    std::cout << '\n';
}

int main() {
    lab::init_console();
    std::promise<void> finished;
    auto completion = finished.get_future();
    std::thread preparation(DataPreparation, std::move(finished));
    preparation.detach();
    std::thread processing(DataProcessing);
    processing.join();
    completion.get();
}
