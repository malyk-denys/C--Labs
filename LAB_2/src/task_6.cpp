#include "common.hpp"
#include <condition_variable>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

struct WorkQueue {
    std::deque<std::packaged_task<std::uint64_t(std::uint32_t)>> tasks;
    std::deque<std::uint32_t> indices;
    std::mutex mutex;
    std::condition_variable available;
    bool closed = false;
};

void Worker(std::shared_ptr<WorkQueue> queue, std::promise<void> finished) {
    for (;;) {
        std::packaged_task<std::uint64_t(std::uint32_t)> task;
        std::uint32_t n = 0;
        {
            std::unique_lock<std::mutex> lock(queue->mutex);
            queue->available.wait(lock, [&] { return queue->closed || !queue->tasks.empty(); });
            if (queue->tasks.empty()) break; // closed=true і всі завдання виконані.
            task = std::move(queue->tasks.front());
            n = queue->indices.front();
            queue->tasks.pop_front();
            queue->indices.pop_front();
        }
        task(n); // Обчислення поза м'ютексом; виняток потрапить у future.
    }
    finished.set_value_at_thread_exit();
}

int main() {
    lab::init_console();
    auto queue = std::make_shared<WorkQueue>();
    std::promise<void> finished;
    auto completion = finished.get_future();
    std::thread worker(Worker, queue, std::move(finished));
    worker.detach();
    std::vector<std::pair<std::uint32_t, std::future<std::uint64_t>>> results;
    std::cout << "Номери простих чисел (1..10000000), q завершує введення:\n";
    std::uint32_t n = 0;
    while (lab::read_index(n)) {
        std::packaged_task<std::uint64_t(std::uint32_t)> task(lab::nth_prime);
        results.emplace_back(n, task.get_future());
        {
            std::lock_guard<std::mutex> lock(queue->mutex);
            queue->tasks.push_back(std::move(task));
            queue->indices.push_back(n);
        }
        queue->available.notify_one();
    }
    {
        std::lock_guard<std::mutex> lock(queue->mutex);
        queue->closed = true;
    }
    queue->available.notify_one();
    int status = 0;
    for (auto& result : results) {
        try {
            const auto prime = result.second.get();
            std::cout << "prime(" << result.first << ")=" << prime << '\n';
        } catch (const std::exception& error) {
            std::cerr << "Помилка обчислення: " << error.what() << '\n';
            status = 1;
        }
    }
    completion.get();
    std::cout << "Усі завдання виконані; робочий потік завершено\n";
    return status;
}
