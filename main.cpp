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


// Вспомогательные функции для корректного выравнивания UTF-8 строк в консоли
size_t utf8_length(const std::string& str) {
    size_t len = 0;
    for (unsigned char c : str) {
        if ((c & 0xC0) != 0x80) {
            len++;
        }
    }
    return len;
}

std::string pad_right(const std::string& str, size_t width) {
    size_t len = utf8_length(str);
    if (len >= width) return str;
    return str + std::string(width - len, ' ');
}

std::string pad_left(const std::string& str, size_t width) {
    size_t len = utf8_length(str);
    if (len >= width) return str;
    return std::string(width - len, ' ') + str;
}

std::string format_double(double val, int precision) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision) << val;
    return ss.str();
}


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


// Демонстрация контрольного примера с подробным выводом каждого дня
void run_control_example() {
    std::cout << "\n========================================================================================================================\n";
    std::cout << "                         КОНТРОЛЬНЫЙ ПРИМЕР: ДЕМОНСТРАЦИЯ ЛОГИКИ МОДЕЛИ (10 ДНЕЙ)\n";
    std::cout << "========================================================================================================================\n";
    std::cout << "Параметры: Порог заказа <= 20 ед., Партия = 40 ед., Срок доставки = 3 дня (заказ вечером дня t -> утром t+3).\n";
    std::cout << "Начальный остаток дня 1 = 40 ед. Доход = 40 руб./ед., Заказ = 100 руб., Хранение = 1 руб./ед. в конце дня.\n\n";

    const std::vector<int> test_demands = {8, 14, 10, 12, 5, 12, 8, 11, 6, 7};
    int stock = INITIAL_STOCK;
    bool order_in_transit = false;
    int order_arrival_day = -1;
    int order_counter = 0;

    double cum_revenue = 0.0;
    double cum_order_cost = 0.0;
    double cum_holding_cost = 0.0;
    double cum_profit = 0.0;
    int cum_sales = 0;
    int cum_lost = 0;

    std::cout << pad_right("День", 6)
              << pad_right("Доставка", 10)
              << pad_right("Склад-утро", 12)
              << pad_right("Спрос", 8)
              << pad_right("Продано", 9)
              << pad_right("Потери", 8)
              << pad_right("Остаток", 10)
              << pad_right("Хранение", 10)
              << pad_right("Заказ", 8)
              << pad_right("Прибыль/день", 14)
              << pad_right("Накопл.прибыль", 16)
              << "Статус заказа\n";
    std::cout << std::string(120, '-') << "\n";

    for (int day = 1; day <= 10; ++day) {
        int delivery = 0;
        std::string order_status = "-";

        // 1. Утренний приём поставки
        if (order_in_transit && day == order_arrival_day) {
            delivery = ORDER_BATCH_SIZE;
            stock += ORDER_BATCH_SIZE;
            order_in_transit = false;
            order_arrival_day = -1;
            order_status = "Поставка принята (+40 ед.)";
        }

        int stock_morning = stock;

        // 2. Продажи
        int demand = test_demands[day - 1];
        int sold = std::min(stock, demand);
        int lost = demand - sold;
        stock -= sold;

        cum_sales += sold;
        cum_lost += lost;

        // 3. Затраты и доходы дня
        double day_revenue = sold * PROFIT_PER_UNIT;
        double day_holding = stock * HOLDING_COST_PER_UNIT;
        double day_order_cost = 0.0;

        // 4. Проверка дозаказа
        if (!order_in_transit && stock <= 20) {
            order_in_transit = true;
            order_arrival_day = day + LEAD_TIME_DAYS;
            day_order_cost = ORDER_FIXED_COST;
            order_counter++;
            order_status = "Заказ #" + std::to_string(order_counter) + " (прибытие утром дня " + std::to_string(order_arrival_day) + ")";
        } else if (order_in_transit) {
            order_status = "В пути (прибытие утром дня " + std::to_string(order_arrival_day) + ")";
        }

        double day_profit = day_revenue - day_order_cost - day_holding;
        cum_revenue += day_revenue;
        cum_order_cost += day_order_cost;
        cum_holding_cost += day_holding;
        cum_profit += day_profit;

        std::string deliv_str = (delivery > 0) ? ("+" + std::to_string(delivery)) : "0";

        std::cout << pad_right(std::to_string(day), 6)
                  << pad_right(deliv_str, 10)
                  << pad_right(std::to_string(stock_morning), 12)
                  << pad_right(std::to_string(demand), 8)
                  << pad_right(std::to_string(sold), 9)
                  << pad_right(std::to_string(lost), 8)
                  << pad_right(std::to_string(stock), 10)
                  << pad_right(std::to_string(static_cast<int>(day_holding)), 10)
                  << pad_right(std::to_string(static_cast<int>(day_order_cost)), 8)
                  << pad_right(std::to_string(static_cast<int>(day_profit)), 14)
                  << pad_right(std::to_string(static_cast<int>(cum_profit)), 16)
                  << order_status << "\n";
    }

    std::cout << std::string(120, '-') << "\n";
    std::cout << "ИТОГИ КОНТРОЛЬНОГО ПРИМЕРА ЗА 10 ДНЕЙ:\n";
    std::cout << "  - Всего поступило спроса: " << (cum_sales + cum_lost) << " ед.\n";
    std::cout << "  - Продано единиц: " << cum_sales << " ед. (Выручка: " << cum_sales << " * 40 = " << cum_revenue << " руб.)\n";
    std::cout << "  - Потерянный спрос: " << cum_lost << " ед. (из-за нехватки товара в дни 4 и 9)\n";
    std::cout << "  - Оформлено заказов: " << order_counter << " шт. (Затраты: " << order_counter << " * 100 = " << cum_order_cost << " руб.)\n";
    std::cout << "  - Затраты на хранение суммарно: " << cum_holding_cost << " руб.\n";
    std::cout << "  - ИТОГОВАЯ ЧИСТАЯ ПРИБЫЛЬ: " << cum_revenue << " - " << cum_order_cost << " - " << cum_holding_cost 
              << " = " << cum_profit << " руб.\n";
    std::cout << "\nКлючевые выводы по контрольному примеру:\n";
    std::cout << "  1) Доставка через 3 дня подтверждена: заказ вечером дня 2 прибыл утром дня 5 (ровно 3 дня).\n";
    std::cout << "     Аналогично, заказ вечером дня 7 прибыл утром дня 10.\n";
    std::cout << "  2) Блокировка повторных заказов работает: в дни 3 и 4 остаток был <= 20, но заказ не дублировался,\n"
              << "     так как предыдущий заказ уже находился в пути.\n";
    std::cout << "  3) Корректность начисления затрат: плата за хранение начисляется строго на фактический остаток в конце дня,\n"
              << "     при нулевом остатке плата за хранение равна 0 руб.\n";
    std::cout << "========================================================================================================================\n\n";
}


int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::cout << "========================================================================================\n";
    std::cout << "    КУРСОВАЯ РАБОТА: ИССЛЕДОВАНИЕ СПОСОБОВ ПОПОЛНЕНИЯ ТОВАРНЫХ ЗАПАСОВ (ВАРИАНТ № 10)\n";
    std::cout << "========================================================================================\n";

    run_control_example();
    return 0;
}
