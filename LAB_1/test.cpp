#include <iostream>
#include <thread>

void hello()
{
    std::cout << "Thread works!" << std::endl;
}

int main()
{
    std::thread t(hello);
    t.join();

    std::cout << "Program finished!" << std::endl;

    return 0;
}