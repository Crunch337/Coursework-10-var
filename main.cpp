/**
 * ==============================================================================================
 * Курсовая работа: Вариант № 10
 * Тема: Исследование способов пополнения товарных запасов магазина
 * 
 * Описание модели:
 * - Моделируется работа магазина с одним видом товара на протяжении года (365 дней).
 * - Начальный запас на складе: 40 единиц.
 * - Ежедневный цикл:
 *     1) Утром: приём поставки (если подошёл срок доставки, +40 единиц).
 *     2) Днём: удовлетворение случайного спроса. Необслуженный спрос = потерянные продажи.
 *     3) Вечером: начисление затрат на хранение (1 руб./ед. остатка) и проверка условия дозаказа.
 * - Правило дозаказа: если текущий остаток <= порогу (10, 20 или 30) и в данный момент нет заказа
 *   в пути, оформляется заказ объёмом 40 единиц.
 * - Стоимость заказа: 100 руб.
 * - Доставка: заказ вечером дня t прибывает утром дня t+3 (ровно через 3 дня).
 * - Ограничение на заказы: после 362-го дня заказы не оформляются (чтобы поставки не выходили за год).
 * - Экономика:
 *     - Выручка от продажи единицы (после вычета закупочной стоимости): 40 руб.
 *     - Затраты на заказ: 100 руб. за поставку.
 *     - Затраты на хранение: 1 руб. за единицу в конце дня.
 * - Исследуются 2 уровня спроса:
 *     1) Обычный спрос: U[0, 8] единиц в день.
 *     2) Повышенный спрос: U[0, 12] единиц в день.
 * - Проводится 1000 независимых экспериментов (по 365 дней). Для каждого эксперимента
 *   все три правила (пороги 10, 20, 30) тестируются на одной и той же реализации спроса (Common Random Numbers).
 * ==============================================================================================
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>
#include <random>
#include <numeric>
#include <cmath>
#include <fstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

// Константы модели
constexpr int TOTAL_DAYS = 365;
constexpr int INITIAL_STOCK = 40;
constexpr int ORDER_BATCH_SIZE = 40;
constexpr int LEAD_TIME_DAYS = 3;
constexpr int LAST_ORDER_DAY = 362;

constexpr double PROFIT_PER_UNIT = 40.0;       // Рублей за проданную единицу товара
constexpr double ORDER_FIXED_COST = 100.0;     // Фиксированная стоимость оформления поставки (руб.)
constexpr double HOLDING_COST_PER_UNIT = 1.0;  // Стоимость хранения единицы товара в конце дня (руб.)


// Структура результатов одного 365-дневного прогона
struct SingleRunResult {
    int total_demand = 0;
    int total_sales = 0;
    int total_lost_sales = 0;
    int num_orders = 0;
    double revenue = 0.0;
    double order_cost = 0.0;
    double holding_cost = 0.0;
    double net_profit = 0.0;
    double avg_stock = 0.0;
    double fill_rate = 0.0; // % удовлетворенного спроса
};


// Функция симуляции одного года при заданной последовательности спроса
SingleRunResult simulate_year(const std::vector<int>& demand_series, int threshold) {
    int stock = INITIAL_STOCK;
    bool order_in_transit = false;
    int order_arrival_day = -1;

    int total_demand = 0;
    int total_sales = 0;
    int total_lost_sales = 0;
    int num_orders = 0;
    double holding_cost = 0.0;
    long long stock_day_sum = 0;

    for (int day = 1; day <= TOTAL_DAYS; ++day) {
        // 1. Утренний приём поставки
        if (order_in_transit && day == order_arrival_day) {
            stock += ORDER_BATCH_SIZE;
            order_in_transit = false;
            order_arrival_day = -1;
        }

        // 2. Дневные продажи
        int demand = demand_series[day - 1];
        total_demand += demand;

        int sold = std::min(stock, demand);
        int lost = demand - sold;
        stock -= sold;

        total_sales += sold;
        total_lost_sales += lost;

        // 3. Вечерний учёт хранения
        holding_cost += stock * HOLDING_COST_PER_UNIT;
        stock_day_sum += stock;

        // 4. Вечерняя проверка необходимости дозаказа
        if (day <= LAST_ORDER_DAY && !order_in_transit && stock <= threshold) {
            order_in_transit = true;
            order_arrival_day = day + LEAD_TIME_DAYS;
            num_orders++;
        }
    }

    SingleRunResult res;
    res.total_demand = total_demand;
    res.total_sales = total_sales;
    res.total_lost_sales = total_lost_sales;
    res.num_orders = num_orders;
    res.revenue = total_sales * PROFIT_PER_UNIT;
    res.order_cost = num_orders * ORDER_FIXED_COST;
    res.holding_cost = holding_cost;
    res.net_profit = res.revenue - res.order_cost - res.holding_cost;
    res.avg_stock = static_cast<double>(stock_day_sum) / TOTAL_DAYS;
    res.fill_rate = (total_demand > 0) ? (100.0 * total_sales / total_demand) : 100.0;

    return res;
}


int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::cout << "Базовая модель управления товарными запасами (Вариант 10)\n";
    std::cout << "Тестовый прогон на 365 дней (спрос U[0, 8], порог <= 20 ед.)...\n";

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 8);
    std::vector<int> demands(TOTAL_DAYS);
    for (int i = 0; i < TOTAL_DAYS; ++i) {
        demands[i] = dist(rng);
    }

    SingleRunResult res = simulate_year(demands, 20);
    std::cout << "Результаты тестового года:\n";
    std::cout << "  Спрос: " << res.total_demand << " ед.\n";
    std::cout << "  Продано: " << res.total_sales << " ед.\n";
    std::cout << "  Потери: " << res.total_lost_sales << " ед.\n";
    std::cout << "  Заказов: " << res.num_orders << "\n";
    std::cout << "  Выручка: " << res.revenue << " руб.\n";
    std::cout << "  Затраты на хранение: " << res.holding_cost << " руб.\n";
    std::cout << "  Затраты на заказы: " << res.order_cost << " руб.\n";
    std::cout << "  Чистая прибыль: " << res.net_profit << " руб.\n";
    std::cout << "  Средний остаток: " << res.avg_stock << " ед.\n";
    return 0;
}
