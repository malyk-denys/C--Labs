#include "common.hpp"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

std::mutex state_mutex;
std::condition_variable event;
std::condition_variable all_waiting;
int i = 0;
int waiting = 0;

void Waits(int id) {
    std::unique_lock<std::mutex> lock(state_mutex);
    std::cout << "Потік " << id << ": початок очікування\n";
    ++waiting;
    all_waiting.notify_one();
    event.wait(lock, [] { return i == 1; });
    std::cout << "Потік " << id << ": очікування завершене, i=" << i << '\n';
}

void Awake() {
    {
        std::unique_lock<std::mutex> lock(state_mutex);
        all_waiting.wait(lock, [] { return waiting == 3; });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        std::cout << "Awake: перше notify_all, i=0\n";
        event.notify_all();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        i = 1;
        std::cout << "Awake: друге notify_all, i=1\n";
    }
    event.notify_all();
}

int main() {
    lab::init_console();
    std::thread t1(Waits, 1), t2(Waits, 2), t3(Waits, 3);
    std::thread awake(Awake);
    awake.join();
    t1.join(); t2.join(); t3.join();
}
