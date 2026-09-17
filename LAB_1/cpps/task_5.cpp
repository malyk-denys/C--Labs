#include <iostream>
#include <thread>
#include <list>
#include <algorithm>
#include <mutex>
#include <future>
#include <vector>
#include <utility>

std::list<int> l;
std::mutex listMutex;

void AddToList(int value)
{
    std::lock_guard<std::mutex> guard(listMutex);

    l.push_back(value);

    std::cout << "Added: " << value << std::endl;
}

void ListContains(int value)
{
    std::lock_guard<std::mutex> guard(listMutex);

    bool found =
        std::find(l.begin(), l.end(), value) != l.end();

    std::cout << "Check: " << value
              << (found ? " found" : " not found")
              << std::endl;
}

int main()
{
    int start = 10;
    int target = 15;

    std::vector<std::future<void>> results;

    for (int i = 0; i < 10; ++i)
    {
        std::packaged_task<void()> addTask(
            [value = start + i]()
            {
                AddToList(value);
            }
        );

        results.push_back(addTask.get_future());

        std::thread addThread(std::move(addTask));
        addThread.detach();

        std::packaged_task<void()> checkTask(
            [target]()
            {
                ListContains(target);
            }
        );

        results.push_back(checkTask.get_future());

        std::thread checkThread(std::move(checkTask));
        checkThread.detach();
    }

    // Очікуємо завершення всіх операцій.
    for (auto& result : results)
    {
        result.get();
    }

    std::cout << "Final list: ";

    for (int value : l)
    {
        std::cout << value << ' ';
    }

    std::cout << "\nSize: " << l.size() << std::endl;

    return 0;
}