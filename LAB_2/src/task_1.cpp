#include "common.hpp"
#include <chrono>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

std::queue<std::int64_t> data_queue;
std::mutex data_mutex;
bool ready = false;

void DataPreparation(std::promise<void> finished) {
    std::cout << "Цілі числа через пробіл; q завершує введення:\n";
    std::string token;
    while (std::cin >> token && token != "q" && token != "Q") {
        std::int64_t value = 0;
        if (!lab::parse_integer(token, value)) {
            std::cout << "Пропущено некоректне число: " << token << '\n';
            continue;
        }
        std::lock_guard<std::mutex> lock(data_mutex);
        data_queue.push(value);
    }
    {
        std::lock_guard<std::mutex> lock(data_mutex);
        ready = true;
    }
    finished.set_value_at_thread_exit();
}

void DataProcessing() {
    std::unique_lock<std::mutex> lock(data_mutex);
    while (!ready) {
        lock.unlock(); // Не тримаємо м'ютекс під час сну.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        lock.lock();
    }
    std::queue<std::int64_t> numbers;
    numbers.swap(data_queue);
    lock.unlock();
    std::cout << "Прості числа:";
    while (!numbers.empty()) {
        const auto value = numbers.front();
        numbers.pop();
        if (lab::is_prime(value)) std::cout << ' ' << value;
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
    completion.get(); // detach не скасовує вимог до часу життя даних.
}
