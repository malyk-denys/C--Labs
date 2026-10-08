#include "common.hpp"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

std::mutex state_mutex;
std::condition_variable event, progress;
int i = 0;
int waiting = 0;
bool delivered = false;
bool stopping = false;

void Waits(int id) {
    std::unique_lock<std::mutex> lock(state_mutex);
    std::cout << "Thread" << id << ": початок очікування\n";
    ++waiting;
    progress.notify_all();
    event.wait(lock, [] { return (i == 1 && !delivered) || stopping; });
    if (stopping) return; // Службове завершення не є обробкою події.
    delivered = true; // Одну подію обробляє лише один потік, навіть при хибному пробудженні.
    std::cout << "Повідомлення з потоку " << id << '\n';
    progress.notify_all();
}
void Thread1() { Waits(1); }
void Thread2() { Waits(2); }
void Thread3() { Waits(3); }

void Notify() {
    {
        std::unique_lock<std::mutex> lock(state_mutex);
        progress.wait(lock, [] { return waiting == 3; });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::lock_guard<std::mutex> lock(state_mutex);
    std::cout << "Notify: notify_one та i=1 під одним м'ютексом\n";
    event.notify_one();
    i = 1; // Пробуджений потік отримає м'ютекс лише після цього запису.
}

int main() {
    lab::init_console();
    std::thread t1(Thread1), t2(Thread2), t3(Thread3), notifier(Notify);
    notifier.join();
    {
        std::unique_lock<std::mutex> lock(state_mutex);
        progress.wait(lock, [] { return delivered; });
        stopping = true;
        std::cout << "Службове завершення решти потоків\n";
    }
    event.notify_all(); // Інакше join двох непробуджених потоків зависне.
    t1.join(); t2.join(); t3.join();
}
