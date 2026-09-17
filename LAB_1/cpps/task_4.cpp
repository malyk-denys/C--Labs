#include <iostream>
#include <thread>
#include <list>
#include <algorithm>
#include <chrono>
#include <mutex>

std::list<int> l;
std::mutex listMutex;

void AddToList(int start)
{
    for (int i = 0; i < 10; ++i)
    {
        int value = start + i;

        listMutex.lock();

        l.push_back(value);
        std::cout << "Added: " << value << std::endl;

        listMutex.unlock();

        std::this_thread::sleep_for(
            std::chrono::milliseconds(30)
        );
    }
}

void ListContains(int value)
{
    for (int i = 0; i < 10; ++i)
    {
        listMutex.lock();

        bool found =
            std::find(l.begin(), l.end(), value) != l.end();

        std::cout << "Check " << i + 1
                  << ": " << value
                  << (found ? " found" : " not found")
                  << std::endl;

        listMutex.unlock();

        std::this_thread::sleep_for(
            std::chrono::milliseconds(40)
        );
    }
}

int main()
{
    int start = 10;
    int target = 15;

    std::thread t1(AddToList, start);
    std::thread t2(ListContains, target);

    t1.join();
    t2.join();

    std::cout << "Final list: ";

    for (int value : l)
    {
        std::cout << value << ' ';
    }

    std::cout << std::endl;

    return 0;
}