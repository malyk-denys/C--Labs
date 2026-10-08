# Лабораторна робота №2

**Тема:** Синхронізація паралельних операцій за допомогою механізму подій та механізму майбутніх результатів.

**Заклад освіти:** ____________________  
**Кафедра / дисципліна:** ____________________  
**Виконав(ла):** ____________________  
**Група:** ____________________  
**Перевірив(ла):** ____________________  
**Місто:** ____________________  
**Рік:** 2026

> Реквізити титульної сторінки заповнює здобувач. Звіт підготовлений у Markdown для перегляду у VS Code (Ctrl+Shift+V). Початкові вимоги містяться в `Lab-2.pdf` у цій папці. Фактичний журнал перевірок: `verification.txt`.

## 1. Мета роботи

Навчитися синхронізувати паралельні операції за допомогою механізму подій та механізму майбутніх результатів. Практично дослідити `std::mutex`, `std::unique_lock`, `std::condition_variable`, `std::async`, `std::packaged_task`, `std::promise` та `std::future`.

## 2. Короткі теоретичні відомості

Потік (`thread`) — послідовність виконання команд усередині процесу. Потоки одного процесу можуть звертатися до спільних даних. Якщо один потік змінює дані, а інший читає або змінює їх без належної синхронізації, виникає гонка даних і невизначена поведінка. М'ютекс (`mutex`) дозволяє лише одному потокові виконувати захищену ділянку. `lock_guard` автоматично звільняє блокування наприкінці області видимості; `unique_lock` додатково дозволяє явно звільняти та повторно захоплювати м'ютекс.

Умовна змінна (`condition_variable`) дозволяє очікувати на виконання умови без періодичного опитування. Виклик `wait(lock, predicate)` перевіряє предикат під м'ютексом. Якщо умова хибна, він атомарно звільняє м'ютекс і переходить до очікування. Після пробудження м'ютекс знову захоплюється й умова перевіряється повторно. Предикат потрібний через можливі спонтанні пробудження та зміни стану іншими потоками. `notify_one()` пробуджує один потік, що очікує; `notify_all()` — усі. Повідомлення не замінює зміни захищених даних і не є збереженою подією.

`future<T>` надає доступ до результату, який буде готовий пізніше. `get()` очікує, повертає результат або повторно викидає збережений виняток; для звичайного `future` його викликають один раз. `async(launch::async, ...)` запускає роботу в окремому потоці, а `async(launch::deferred, ...)` відкладає її до першого блокувального очікування результату. `packaged_task` поєднує функцію з майбутнім результатом. `promise` дозволяє явно записати результат або виняток у спільний стан, який читає `future`. Пара `promise<bool>/future<bool>` може передавати одноразовий сигнал.

`join()` очікує завершення потоку. `detach()` від'єднує об'єкт `thread`, але не подовжує час життя даних і не гарантує завершення роботи до виходу з процесу. У цій реалізації завершення від'єднаних потоків підтверджується через `promise<void>::set_value_at_thread_exit()` і відповідні `future<void>`.

## 3. Середовище та алгоритми

- ОС: Windows; збірка і перевірка через PowerShell.
- Компілятор: GCC 15.2.0, стандарт C++17.
- Ключі: `-std=c++17 -O2 -g -Wall -Wextra -Wpedantic -pthread -static` та UTF-8.
- Редактор: підготовлено конфігурації VS Code для збірки, запуску та GDB. Графічний запуск F5 у VS Code не перевірявся; GDB успішно прочитав символи зібраної програми.
- Перевірка проведена 03.10.2026. Збірка всіх семи програм завершилася без повідомлень про помилки чи попередження.

Для перевірки окремого числа `x` виключаються значення менші за 2 та парні числа, крім 2, після чого перебираються непарні дільники до квадратного кореня. Умова `d <= x / d` еквівалентна `d*d <= x` для додатних значень і не спричиняє переповнення добутку.

Для знаходження n-го простого числа застосовано решето Ератосфена: у бітовому масиві викреслюються кратні простих чисел, після чого підраховуються невикреслені числа. Початкова межа для `n >= 6` оцінюється як `n * (ln(n) + ln(ln(n)))` із запасом; якщо числа не знайдено, межа збільшується. Невеликі `n` обробляються з межею 15. Усі масиви локальні, тому одночасні виклики незалежні. Нумерація починається з 1: `prime(1)=2`.

## 4. Хід роботи та результати

### 4.1. Пункт 1.2.1 — прапорець і періодичне очікування

`DataPreparation` читає довільну кількість цілих чисел до `q` або EOF та поміщає їх у глобальну чергу під м'ютексом. Після завершення вводу виставляє глобальний `ready`. Потік від'єднується через `detach()`.

`DataProcessing` перевіряє прапорець через `unique_lock`. Якщо дані не готові, звільняє м'ютекс, спить 100 мс і повторно захоплює його. Сон під захопленим м'ютексом блокував би виробника, тому `unlock()` принципово важливий. Після готовності черга переноситься в локальну змінну і обробляється без тривалого утримання м'ютекса. Головний потік виконує `join()` споживача та очікує підтвердження завершення виробника.

Ввід: `-5 0 1 2 3 4 17 25 97 q`.

```text
Прості числа: 2 3 17 97
```

Перевірено також порожній ввід, EOF і некоректні токени. Періодичне опитування просте, але спричиняє зайві пробудження та додаткову затримку до наступної перевірки (номінально до 100 мс, фактична затримка залежить і від планувальника).

### 4.2. Пункт 1.2.2 — notify_all та предикат

Три потоки виконують `Waits(id)` і очікують із предикатом `i == 1`. Додаткова умовна змінна забезпечує, що всі три потоки розпочали очікування до експерименту. `Awake` спить 100 мс, надсилає перше `notify_all()` при `i=0`, знову спить 100 мс, змінює `i` на 1 і надсилає друге повідомлення.

Проведено **10 запусків**. У кожному було три повідомлення про завершення очікування, і всі вони з'являлися після другого повідомлення `Awake`. Перше повідомлення не дозволяє потокам пройти `wait`, оскільки предикат залишається хибним. Головний потік очікує `Awake` і всі три потоки `Waits` через `join()`.

Порядок повідомлень потоків залежить від планувальника: `notify_all()` не встановлює черговості доступу до м'ютекса. Детальні послідовності кожного запуску збережені в журналі перевірки.

### 4.3. Пункт 1.2.3 — notify_one

Створено функції `Thread1`, `Thread2`, `Thread3` та `Notify`. Після готовності трьох очікувачів `Notify` спить 100 мс, викликає `notify_one()` і присвоює `i=1` під одним блокуванням. Пробуджений потік не може перевірити `i` до звільнення цього м'ютекса, тому побачить оновлене значення.

Додано захищену змінну `delivered`: одна робоча подія може бути оброблена лише раз, зокрема за можливих спонтанних пробуджень. Після одного повідомлення головний потік установлює службовий прапорець `stopping` і надсилає `notify_all()`, щоб два інші потоки завершилися без робочих повідомлень. Без цього доповнення їхні `join()` могли б очікувати нескінченно.

У **10 із 10 запусків** отримано рівно одне робоче повідомлення. На цьому ПК в усіх десяти спостереженнях це було:

```text
Повідомлення з потоку 1
```

Цей результат не доводить, що стандарт гарантує вибір першого потоку. Порядок вибору очікувача не заданий. `notify_one()` застосовний, коли одну доступну одиницю роботи має обробити один споживач.

### 4.4. Пункт 1.2.4 — черга й умовна змінна

Прапорець готовності та опитування замінено на `data_ready.wait(lock, [] { return !data_queue.empty(); })`. Щоб зберегти обробку лише після завершення всього вводу, виробник спочатку збирає локальний пакет, додає маркер кінця `std::nullopt`, а потім передає пакет глобальній черзі під м'ютексом і надсилає `notify_one()`.

Тип елемента черги — `optional<int64_t>`: наявне значення означає число, відсутнє — кінець. Навіть якщо користувач одразу введе `q`, черга отримає маркер і споживач не зависне. Жодне допустиме ціле число не використовується як службовий код.

Для того самого вводу отримано `Прості числа: 2 3 17 97`. Перевірки помилкових токенів, EOF та порожньої черги також пройдені. На відміну від пункту 1.2.1, немає періодичного пробудження кожні 100 мс.

### 4.5. Пункт 1.2.5 — deferred та async

Програма читає `n`, створює `future` через `async`, пропонує обчислити `sqrt(n)`, `sin(n)` або `ln(n)`, а потім отримує n-те просте число. Синус обчислюється для аргументу в радіанах. Експеримент виконується спочатку з `launch::deferred`, потім із `launch::async`. У режимі `deferred` обчислення простого числа під час вводу користувача ще не відбувається — саме це ілюструє різницю політик.

Для відтворюваного вимірювання використано режим `--benchmark 1000000`, який в обох випадках імітує 500 мс незалежної роботи до `get()`. Це штучна затримка, а не реальний час вводу користувача. Усі шість обчислень дали `prime(1000000)=15485863`.

| Повтор | Політика | Очікування get(), мс | Загальний час, мс |
|---|---|---:|---:|
| 1 | deferred | 39.815 | 554.375 |
| 1 | async | 0.043 | 509.636 |
| 2 | deferred | 39.382 | 549.304 |
| 2 | async | 0.031 | 505.574 |
| 3 | deferred | 39.815 | 549.317 |
| 3 | async | 0.040 | 505.719 |

Час виміряно `steady_clock`. `wait_ms` охоплює лише виклик `get()`, а `total_ms` включає запуск, іншу роботу й отримання результату. У `deferred` обчислення починається всередині `get()`. У `async` воно перекривається з 500 мс іншої роботи та в цих запусках закінчується до `get()`. Це не означає, що саме обчислення стало у тисячу разів швидшим: скоротився час залишкового очікування. Для малого `n` створення окремого потоку може коштувати дорожче за саме обчислення; інтерактивні перевірки малих значень також записані в журналі.

### 4.6. Пункт 1.2.6 — два деки та packaged_task

У спільному об'єкті `WorkQueue` є два деки: `tasks` з `packaged_task<uint64_t(uint32_t)>` і `indices` з номерами `n`. Пари елементів додаються та вилучаються під одним м'ютексом, тому відповідність «задача — аргумент» зберігається. Фоновий потік очікує на умовній змінній, переміщує задачу з дека, звільняє м'ютекс і викликає `task(n)`.

Головний потік зберігає відповідні `future` та після `q` закриває чергу для нового вводу. Робочий потік завершується лише тоді, коли черга закрита й усі задачі вилучені та виконані. `shared_ptr` підтримує час життя черги для від'єднаного виконавця, а окремий `future<void>` підтверджує вихід потоку. Обчислення не блокують введення наступних номерів. Результати друкуються головним потоком у порядку введення після завершення опитування.

Ввід: `1 2 10 100 q`.

```text
prime(1)=2
prime(2)=3
prime(10)=29
prime(100)=541
Усі завдання виконані; робочий потік завершено
```

Додатково перевірено чергу з 50 завдань: усі 50 результатів надруковано, останній `prime(50)=229`. Перевірено закриття порожньої черги та завершення за EOF.

### 4.7. Пункт 1.2.7 — promise/future та залежні потоки

Глобальних змінних немає. Всі promises, futures, м'ютекс друку та аргументи створюються в `main` і передаються потокам. Обидві потокові функції мають тип повернення `void` і від'єднуються через `detach()`.

Перший потік обчислює `prime(n)`, заповнює відповідний promise, передає булевий сигнал `true` другому потокові, а далі обчислює `prime(10*n)`. Другий потік отримує сигнал через `future<bool>::get()`, очікує дві секунди та виводить `sqrt(n)`. Головний потік друкує обидва прості числа та очікує підтвердження завершення обох потоків. Потенційні помилки обчислення передаються через майбутні результати й повідомлення про завершення.

Для `n=10` фактично отримано:

```text
prime(10)=29
prime(100)=541
sqrt(10)=3.16228
```

Тривалість усього тестового процесу — 2271 мс, включно із запуском процесу та роботою потоків. Тест окремо перевірив, що програма не завершується до двосекундного очікування другого потоку. Порядок появи `sqrt(n)` і `prime(10*n)` загалом залежить від тривалості обчислення; булевий сигнал залежить від готовності першого простого числа, а не від того, чи головний потік уже встиг його надрукувати.

### 4.8. Перевірка результату

Виконано `verify.cmd`. Підсумок: **ALL CHECKS PASSED: 100**. Це 100 перевірених умов, включно з кодами завершення процесів, а не 100 окремих програм. Перевірено контрольні значення простих чисел, діапазони вводу, помилкові токени, переповнення при розборі рядка, EOF, порожній ввід, повторні конкурентні запуски, закриття черги та завершення фонових потоків.

Тайм-аут кожного процесу — 30 секунд. Для вимірювань `async` перевіряється математичний результат, але не нав'язується конкретний час: він залежить від навантаження. Динамічний аналізатор гонок даних не застосовувався; коректність синхронізації обґрунтована захистом стану м'ютексами та протоколами очікування і додатково перевірена повторними запусками.

## 5. Декларація використання генеративного ШІ

Під час підготовки цієї роботи використано OpenAI Codex для аналізу методичних вимог, створення програм C++, сценаріїв збірки та автоматичної перевірки, налаштувань VS Code й тексту звіту. Компіляція та тестові запуски виконані інструментами Codex на цьому комп'ютері; їхні результати наведено в журналі `verification.txt`.

Цей текст не стверджує, що весь код написаний здобувачем самостійно. Перед поданням здобувач має перевірити роботу, розібратися в реалізації, заповнити власні реквізити й підтвердити відповідальність за поданий результат відповідно до правил навчального закладу.

## 6. Висновки

Реалізовано всі сім програм і досліджено кілька способів синхронізації. Прапорець із періодичним сном є простим, але додає затримку та потребує правильного блокування. Умовна змінна дозволяє чекати на стан без опитування. `notify_all()` доречний для загальної зміни стану, а `notify_one()` — для надання роботи одному очікувачу; порядок вибору потоків не гарантується.

`future` зручний для одноразового отримання результату або винятку. `async` найпростіший для окремої обчислювальної задачі; `packaged_task` зручний для черги завдань із власним виконавцем; `promise` дозволяє явно зв'язати етапи роботи різних потоків. На цьому ПК `launch::async` дозволив майже повністю приховати час знаходження мільйонного простого числа за 500 мс іншої роботи, тоді як `deferred` почав обчислення лише під час `get()`.

Коректне завершення потоків є частиною синхронізації. Самі виклики `detach()` або `notify_one()` не гарантують завершення програми. Для від'єднаних потоків використано підтвердження виходу, а для черг і очікувачів — явно визначені умови завершення.

## 7. Відповіді на контрольні запитання

1. **Як забезпечується розпаралелювання задач в ОС?** Планувальник розподіляє потоки між логічними процесорами. На кількох ядрах можливе одночасне виконання, а на одному — почергове виконання з перемиканням контексту. Конкурентне виконання не завжди означає фізичний паралелізм.
2. **Як забезпечено управління потоками у C++?** Через `std::thread` із `<thread>`, його `join()`, `detach()`, `joinable()` та функції `std::this_thread`; `std::async` пропонує вищий рівень керування обчисленням і результатом.
3. **Що таке м'ютекс?** Засіб взаємного виключення: лише один потік може володіти конкретним м'ютексом одночасно. Усі конфліктні доступи до даних мають дотримуватися одного протоколу блокування. Звільнення м'ютекса й наступне захоплення забезпечують видимість захищених змін.
4. **Як створити м'ютекс?** Підключити `<mutex>` і оголосити `std::mutex m;`. Для безпечного автоматичного звільнення використати `std::lock_guard<std::mutex>` або `std::unique_lock<std::mutex>`.
5. **Що таке умовна змінна?** Об'єкт для блокувального очікування на зміну стану, який захищений м'ютексом. Сам стан зберігається у змінних програми, а не в повідомленні умовної змінної.
6. **Як створити умовну змінну?** Підключити `<condition_variable>` та оголосити `std::condition_variable cv;`. Вона працює з `unique_lock<mutex>`. `condition_variable_any` підтримує й інші сумісні типи блокувань.
7. **Як змусити потік очікувати?** Захопити м'ютекс через `unique_lock` і викликати `cv.wait(lock, [&] { return ready; });`. Інший потік змінює `ready` під тим самим м'ютексом і викликає `notify_one()` або `notify_all()`.
8. **Що таке об'єкт майбутньої події?** `std::future<T>` представляє доступ до майбутнього результату або винятку в спільному стані. `shared_future<T>` дозволяє кільком споживачам отримувати той самий результат.
9. **Як запустити асинхронне обчислення з майбутнім результатом?** Наприклад, `auto f = std::async(std::launch::async, function, argument);`, після чого `auto result = f.get();`.
10. **Які режими допускає std::async?** `launch::async` — окремий потік; `launch::deferred` — відкладене виконання у потоці, який першим викликає відповідне блокувальне очікування. Без явної політики дозволено обидва варіанти, і реалізація вибирає один.
11. **Чи можна викликати packaged_task безпосередньо?** Так, через `task(args...)`. Це виконує функцію в поточному потоці й записує результат або виняток у пов'язаний спільний стан. Сам `packaged_task` нового потоку не створює.
12. **Як отримати future, пов'язаний із promise?** Викликати `auto f = promise.get_future();` один раз для цього promise. Постачальник використовує `set_value(...)` або `set_exception(...)`, споживач — `f.get()`.

## 8. Повні лістинги

Нижче наведено спільний заголовок і всі сім програм у стані, в якому вони пройшли перевірки.


### src/common.hpp

```cpp
#pragma once

#include <charconv>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

namespace lab {
inline void init_console() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

// from_chars перевіряє весь токен: "12abc" не є числом.
inline bool parse_integer(const std::string& token, std::int64_t& value) {
    if (token.empty()) return false;
    const char* begin = token.data();
    if (*begin == '+') {
        ++begin;
        if (begin == token.data() + token.size() || *begin < '0' || *begin > '9')
            return false;
    }
    const auto result = std::from_chars(begin, token.data() + token.size(), value);
    return result.ec == std::errc{} && result.ptr == token.data() + token.size();
}

inline bool is_prime(std::int64_t value) {
    if (value < 2) return false;
    if (value % 2 == 0) return value == 2;
    // Ділення замість d*d усуває переповнення.
    for (std::int64_t d = 3; d <= value / d; d += 2)
        if (value % d == 0) return false;
    return true;
}

inline bool read_index(std::uint32_t& n, std::uint32_t limit = 10000000) {
    std::string token;
    while (std::cin >> token) {
        if (token == "q" || token == "Q") return false;
        std::int64_t value = 0;
        if (parse_integer(token, value) && value >= 1 && value <= limit) {
            n = static_cast<std::uint32_t>(value);
            return true;
        }
        std::cout << "Некоректний номер. Введіть 1.." << limit << " або q: " << std::flush;
    }
    return false; // EOF також означає завершення вводу.
}

// Решето Ератосфена: O(B log log B) часу, O(B) бітів пам'яті.
// Локальний масив дозволяє одночасні виклики без спільних змінних.
inline std::uint64_t nth_prime(std::uint32_t n) {
    if (n == 0 || n > 10000000) throw std::out_of_range("n must be in 1..10000000");
    const double x = n;
    std::size_t bound = n < 6 ? 15 : static_cast<std::size_t>(x * (std::log(x) + std::log(std::log(x)))) + 16;
    for (;;) {
        std::vector<bool> prime(bound + 1, true);
        prime[0] = prime[1] = false;
        for (std::size_t p = 2; p <= bound / p; ++p)
            if (prime[p])
                for (std::size_t multiple = p * p; multiple <= bound; multiple += p)
                    prime[multiple] = false;
        std::uint32_t count = 0;
        for (std::size_t p = 2; p <= bound; ++p)
            if (prime[p] && ++count == n) return p;
        bound *= 2; // Запасний шлях, якщо початкова межа недостатня.
    }
}
} // namespace lab

```


### src/task_1.cpp

```cpp
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

```


### src/task_2.cpp

```cpp
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

```


### src/task_3.cpp

```cpp
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

```


### src/task_4.cpp

```cpp
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

```


### src/task_5.cpp

```cpp
#include "common.hpp"
#include <chrono>
#include <future>
#include <iomanip>
#include <thread>

using Clock = std::chrono::steady_clock;
double milliseconds(Clock::duration time) {
    return std::chrono::duration<double, std::milli>(time).count();
}

void experiment(std::launch policy, std::uint32_t n, bool benchmark) {
    const char* name = policy == std::launch::deferred ? "deferred" : "async";
    std::cout << "\nРежим " << name << '\n';
    const auto start = Clock::now();
    auto result = std::async(policy, lab::nth_prime, n);
    if (benchmark) {
        // Однакова і явно позначена імітація роботи користувача в обох режимах.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "sqrt(n)=" << std::sqrt(static_cast<double>(n)) << '\n';
    } else {
        std::cout << "Функція від n: 1=sqrt, 2=sin (радіани), 3=ln, 0=пропустити: " << std::flush;
        std::string choice;
        while (std::cin >> choice) {
            if (choice == "0" || choice == "q") break;
            if (choice == "1") std::cout << "sqrt(n)=" << std::sqrt(static_cast<double>(n)) << '\n';
            else if (choice == "2") std::cout << "sin(n)=" << std::sin(static_cast<double>(n)) << '\n';
            else if (choice == "3") std::cout << "ln(n)=" << std::log(static_cast<double>(n)) << '\n';
            else { std::cout << "Введіть 0, 1, 2 або 3: " << std::flush; continue; }
            break;
        }
    }
    const auto before_get = Clock::now();
    const auto prime = result.get();
    const auto finish = Clock::now();
    std::cout << "prime(" << n << ")=" << prime << '\n'
              << std::fixed << std::setprecision(3)
              << "RESULT mode=" << name << " n=" << n << " prime=" << prime
              << " wait_ms=" << milliseconds(finish - before_get)
              << " total_ms=" << milliseconds(finish - start) << '\n';
}

int main(int argc, char* argv[]) {
    lab::init_console();
    try {
        const bool benchmark = argc > 1 && std::string(argv[1]) == "--benchmark";
        std::uint32_t n = 0;
        if (benchmark) {
            std::int64_t value = 1000000;
            if (argc > 3 || (argc == 3 && !lab::parse_integer(argv[2], value)) || value < 1 || value > 10000000)
                throw std::invalid_argument("Usage: task_5 --benchmark [n: 1..10000000]");
            n = static_cast<std::uint32_t>(value);
            std::cout << "Benchmark: 500 ms simulated user work per mode\n";
        } else {
            if (argc != 1) throw std::invalid_argument("Usage: task_5 [--benchmark [n]]");
            std::cout << "Номер простого числа (1..10000000 або q): " << std::flush;
            if (!lab::read_index(n)) return 0;
        }
        experiment(std::launch::deferred, n, benchmark);
        experiment(std::launch::async, n, benchmark);
    } catch (const std::exception& error) {
        std::cerr << "Помилка: " << error.what() << '\n';
        return 1;
    }
}

```


### src/task_6.cpp

```cpp
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

```


### src/task_7.cpp

```cpp
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

```
