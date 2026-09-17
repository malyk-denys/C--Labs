#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <future>
#include <functional>
#include <utility>

class someData
{
public:
    std::string name = "Empty";
    std::string surname = "Empty";
    std::string address = "Empty";
    int age = 0;
};

class exchangePerson
{
private:
    someData data;
    std::mutex dataMutex;

    // Викликається у Swap, коли м'ютекси вже захоплені.
    static void PrintData(
        const char* label,
        const someData& value
    )
    {
        std::cout << label << ": "
                  << value.name << ' '
                  << value.surname << ", "
                  << value.address << ", age = "
                  << value.age << '\n';
    }

public:
    static void JohnDoe(exchangePerson& person)
    {
        std::lock_guard<std::mutex> guard(person.dataMutex);

        person.data.name = "John";
        person.data.surname = "Doe";
        person.data.address = "Unknown";
        person.data.age = 120;
    }

    static void JacobSmith(exchangePerson& person)
    {
        std::lock_guard<std::mutex> guard(person.dataMutex);

        person.data.name = "Jacob";
        person.data.surname = "Smith";
        person.data.address = "Known";
        person.data.age = 1;
    }

    static void Swap(
        exchangePerson& first,
        exchangePerson& second
    )
    {
        // Перевіряємо, чи це не один і той самий об'єкт.
        if (&first == &second)
        {
            return;
        }

        // Захоплюємо обидва м'ютекси з уникненням deadlock.
        std::lock(first.dataMutex, second.dataMutex);

        // М'ютекси вже захоплені.
        // lock_guard забезпечить їх автоматичне звільнення.
        std::lock_guard<std::mutex> firstGuard(
            first.dataMutex,
            std::adopt_lock
        );

        std::lock_guard<std::mutex> secondGuard(
            second.dataMutex,
            std::adopt_lock
        );

        std::cout << "Before swap:\n";
        PrintData("First", first.data);
        PrintData("Second", second.data);

        std::swap(first.data, second.data);

        std::cout << "\nAfter swap:\n";
        PrintData("First", first.data);
        PrintData("Second", second.data);
    }
};

// Запускає операцію у від'єднаному потоці.
// Повертає future для очікування завершення операції.
std::future<void> StartDetached(
    void (*operation)(exchangePerson&),
    exchangePerson& person
)
{
    std::packaged_task<void()> task(
        [operation, &person]()
        {
            operation(person);
        }
    );

    std::future<void> result = task.get_future();

    std::thread worker(std::move(task));
    worker.detach();

    return result;
}

int main()
{
    exchangePerson first;
    exchangePerson second;

    // Для кожного об'єкта запускаємо обидва методи.
    auto firstJohn =
        StartDetached(exchangePerson::JohnDoe, first);

    auto firstJacob =
        StartDetached(exchangePerson::JacobSmith, first);

    auto secondJohn =
        StartDetached(exchangePerson::JohnDoe, second);

    auto secondJacob =
        StartDetached(exchangePerson::JacobSmith, second);

    // Очікуємо завершення всіх чотирьох операцій.
    firstJohn.get();
    firstJacob.get();
    secondJohn.get();
    secondJacob.get();

    // Додатково встановлюємо різні записи,
    // щоб обмін був наочним.
    auto setFirst =
        StartDetached(exchangePerson::JohnDoe, first);

    auto setSecond =
        StartDetached(exchangePerson::JacobSmith, second);

    setFirst.get();
    setSecond.get();

    // Обмін виконується в окремому потоці.
    std::thread swapThread(
        exchangePerson::Swap,
        std::ref(first),
        std::ref(second)
    );

    // Гарантуємо завершення обміну до виходу з main.
    swapThread.join();

    return 0;
}