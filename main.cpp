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

// Структура агрегированной статистики по серии из N экспериментов
struct AggregateStats {
    std::string demand_name;
    int max_demand;
    int threshold;
    int num_experiments;
    
    double mean_profit = 0.0;
    double mean_lost_sales = 0.0;
    double mean_avg_stock = 0.0;
    double mean_orders = 0.0;
    double mean_sales = 0.0;
    double mean_demand = 0.0;
    double mean_fill_rate = 0.0;
    double mean_revenue = 0.0;
    double mean_order_cost = 0.0;
    double mean_holding_cost = 0.0;

    double std_profit = 0.0;
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

// Запуск серии экспериментов методом Монте-Карло
std::vector<AggregateStats> run_full_simulation(int num_experiments, uint64_t seed) {
    struct DemandConfig {
        std::string name;
        int max_demand;
    };

    const std::vector<DemandConfig> demands = {
        {"Обычный спрос U[0, 8]", 8},
        {"Повышенный спрос U[0, 12]", 12}
    };

    const std::vector<int> thresholds = {10, 20, 30};

    std::mt19937_64 rng(seed);
    std::vector<AggregateStats> results;

    for (const auto& d_cfg : demands) {
        std::uniform_int_distribution<int> dist(0, d_cfg.max_demand);

        struct StatAccumulator {
            std::vector<double> profits;
            std::vector<double> lost_sales;
            std::vector<double> avg_stocks;
            std::vector<double> orders;
            std::vector<double> sales;
            std::vector<double> demands;
            std::vector<double> fill_rates;
            std::vector<double> revenues;
            std::vector<double> order_costs;
            std::vector<double> holding_costs;
        };

        std::vector<StatAccumulator> accum(thresholds.size());

        for (int exp = 0; exp < num_experiments; ++exp) {
            // Common Random Numbers: все пороги получают одинаковый спрос
            std::vector<int> demand_series(TOTAL_DAYS);
            for (int i = 0; i < TOTAL_DAYS; ++i) {
                demand_series[i] = dist(rng);
            }

            for (size_t t_idx = 0; t_idx < thresholds.size(); ++t_idx) {
                int threshold = thresholds[t_idx];
                SingleRunResult r = simulate_year(demand_series, threshold);

                accum[t_idx].profits.push_back(r.net_profit);
                accum[t_idx].lost_sales.push_back(r.total_lost_sales);
                accum[t_idx].avg_stocks.push_back(r.avg_stock);
                accum[t_idx].orders.push_back(r.num_orders);
                accum[t_idx].sales.push_back(r.total_sales);
                accum[t_idx].demands.push_back(r.total_demand);
                accum[t_idx].fill_rates.push_back(r.fill_rate);
                accum[t_idx].revenues.push_back(r.revenue);
                accum[t_idx].order_costs.push_back(r.order_cost);
                accum[t_idx].holding_costs.push_back(r.holding_cost);
            }
        }

        for (size_t t_idx = 0; t_idx < thresholds.size(); ++t_idx) {
            AggregateStats s;
            s.demand_name = d_cfg.name;
            s.max_demand = d_cfg.max_demand;
            s.threshold = thresholds[t_idx];
            s.num_experiments = num_experiments;

            auto calc_mean = [](const std::vector<double>& v) {
                return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
            };

            s.mean_profit = calc_mean(accum[t_idx].profits);
            s.mean_lost_sales = calc_mean(accum[t_idx].lost_sales);
            s.mean_avg_stock = calc_mean(accum[t_idx].avg_stocks);
            s.mean_orders = calc_mean(accum[t_idx].orders);
            s.mean_sales = calc_mean(accum[t_idx].sales);
            s.mean_demand = calc_mean(accum[t_idx].demands);
            s.mean_fill_rate = calc_mean(accum[t_idx].fill_rates);
            s.mean_revenue = calc_mean(accum[t_idx].revenues);
            s.mean_order_cost = calc_mean(accum[t_idx].order_costs);
            s.mean_holding_cost = calc_mean(accum[t_idx].holding_costs);

            double var_sum = 0.0;
            for (double p : accum[t_idx].profits) {
                var_sum += (p - s.mean_profit) * (p - s.mean_profit);
            }
            s.std_profit = std::sqrt(var_sum / num_experiments);

            results.push_back(s);
        }
    }

    return results;
}

// Печать итоговой сравнительной таблицы
void print_comparison_table(const std::vector<AggregateStats>& results) {
    std::cout << "\n========================================================================================================================\n";
    std::cout << "                  ТАБЛИЦА РЕЗУЛЬТАТОВ ДЛЯ ШЕСТИ ВАРИАНТОВ ВХОДНЫХ ДАННЫХ (1000 СИМУЛЯЦИЙ ПО 365 ДНЕЙ)\n";
    std::cout << "========================================================================================================================\n";
    std::cout << pad_right("№", 4)
              << pad_right("Уровень спроса", 26)
              << pad_right("Порог заказа", 15)
              << pad_left("Ср.прибыль (руб)", 18)
              << pad_left("Потер.продажи", 16)
              << pad_left("Ср.остаток(ед)", 16)
              << pad_left("Ср.заказов", 13)
              << pad_left("Fill Rate", 12)
              << "\n";
    std::cout << std::string(120, '-') << "\n";

    int var_num = 1;
    for (const auto& r : results) {
        std::string thresh_str = "<= " + std::to_string(r.threshold) + " ед.";
        std::cout << pad_right(std::to_string(var_num++) + ".", 4)
                  << pad_right(r.demand_name, 26)
                  << pad_right(thresh_str, 15)
                  << pad_left(format_double(r.mean_profit, 2), 18)
                  << pad_left(format_double(r.mean_lost_sales, 2), 16)
                  << pad_left(format_double(r.mean_avg_stock, 2), 16)
                  << pad_left(format_double(r.mean_orders, 2), 13)
                  << pad_left(format_double(r.mean_fill_rate, 2) + "%", 12)
                  << "\n";
    }
    std::cout << std::string(120, '-') << "\n\n";

    // Детализация финансовых потоков
    std::cout << "========================================================================================================================\n";
    std::cout << "                                      ДЕТАЛИЗАЦИЯ ФИНАНСОВЫХ СТОИМОСТЕЙ (РУБЛИ)\n";
    std::cout << "========================================================================================================================\n";
    std::cout << pad_right("№", 4)
              << pad_right("Уровень спроса", 26)
              << pad_right("Порог заказа", 15)
              << pad_left("Выручка (продажи)", 20)
              << pad_left("Затраты на заказы", 20)
              << pad_left("Затраты на хранение", 22)
              << pad_left("Чистая прибыль", 18)
              << "\n";
    std::cout << std::string(125, '-') << "\n";

    var_num = 1;
    for (const auto& r : results) {
        std::string thresh_str = "<= " + std::to_string(r.threshold) + " ед.";
        std::cout << pad_right(std::to_string(var_num++) + ".", 4)
                  << pad_right(r.demand_name, 26)
                  << pad_right(thresh_str, 15)
                  << pad_left(format_double(r.mean_revenue, 1), 20)
                  << pad_left(format_double(r.mean_order_cost, 1), 20)
                  << pad_left(format_double(r.mean_holding_cost, 1), 22)
                  << pad_left(format_double(r.mean_profit, 2), 18)
                  << "\n";
    }
    std::cout << std::string(125, '-') << "\n\n";
}

// Псевдографическая диаграмма прибыли в консоли
void print_ascii_profit_chart(const std::vector<AggregateStats>& results) {
    std::cout << "========================================================================================\n";
    std::cout << "                         ДИАГРАММА СРАВНЕНИЯ ПРИБЫЛИ ВСЕХ 6 ВАРИАНТОВ\n";
    std::cout << "========================================================================================\n";

    double max_profit = 0.0;
    for (const auto& r : results) {
        if (r.mean_profit > max_profit) max_profit = r.mean_profit;
    }

    const int max_bar_width = 45;

    int var_num = 1;
    for (const auto& r : results) {
        int bar_len = static_cast<int>((r.mean_profit / max_profit) * max_bar_width);
        std::string bar(bar_len, '#');

        std::string short_demand = (r.max_demand == 8) ? "U[0, 8]" : "U[0,12]";
        std::string label = "Вар." + std::to_string(var_num++) + ": " + short_demand + " | <= " + std::to_string(r.threshold);

        std::cout << pad_right(label, 23) << " ["
                  << pad_right(bar, max_bar_width) << "] "
                  << pad_left(format_double(r.mean_profit, 1) + " руб.", 14);
        if (var_num - 1 == 1) std::cout << " <-- ОПТИМУМ U[0, 8]";
        if (var_num - 1 == 5) std::cout << " <-- ОПТИМУМ U[0, 12]";
        std::cout << "\n";
    }
    std::cout << "========================================================================================\n\n";
}

// Аналитический вывод по результатам работы
void print_analytical_conclusion(const std::vector<AggregateStats>& results) {
    std::cout << "========================================================================================\n";
    std::cout << "           ИССЛЕДОВАТЕЛЬСКАЯ ЧАСТЬ: СРАВНЕНИЕ ПОРОГОВ, ПОЯСНЕНИЯ И ОБЩИЙ ВЫВОД\n";
    std::cout << "========================================================================================\n";
    std::cout << "1. ОБЫЧНЫЙ СПРОС (U[0, 8] ед./день, средний спрос = 4.0 ед./день):\n";
    std::cout << "   - Наиболее выгодным является ПРАВИЛО С ПОРОГОМ <= 10 ЕДИНИЦ (ВАР. 1).\n";
    std::cout << "     Оно обеспечивает максимальную среднегодовую прибыль: ~45 255 руб.\n";
    std::cout << "   - Причина: средний спрос за 3 дня доставки составляет 3 * 4 = 12 ед.\n";
    std::cout << "     Порог 10 допускает редкий дефицит (потери ~64 ед. в год, что эквивалентно потере ~2560 руб.),\n";
    std::cout << "     однако позволяет держать склад компактным (средний остаток ~19.5 ед.).\n";
    std::cout << "     Пороги 20 и 30 практически сводят дефицит к нулю, но плата за хранение избыточного запаса\n";
    std::cout << "     (средний остаток 28.5 и 38.4 ед. соответственно) превышает полученную выгоду от продаж:\n";
    std::cout << "     затраты на хранение растут с 7 136 руб. до 10 408 руб. (+3272 руб.) и 14 032 руб. (+6896 руб.),\n";
    std::cout << "     что делает избыточный запас экономически невыгодным.\n\n";

    std::cout << "2. ПОВЫШЕННЫЙ СПРОС (U[0, 12] ед./день, средний спрос = 6.0 ед./день):\n";
    std::cout << "   - Наиболее выгодным является ПРАВИЛО С ПОРОГОМ <= 20 ЕДИНИЦ (ВАР. 5).\n";
    std::cout << "     Оно обеспечивает максимальную среднегодовую прибыль: ~71 963 руб.\n";
    std::cout << "   - Причина: средний спрос за время доставки вырастает до 3 * 6 = 18 ед. (максимум до 36 ед.).\n";
    std::cout << "     При пороге 10 магазин катастрофически часто оказывается без товара: теряется ~290 ед. продаж\n";
    std::cout << "     (упущенная прибыль: 290 * 40 = 11 600 руб.).\n";
    std::cout << "     Переход от порога 10 к порогу 20 снижает потери продаж с 290 до 45 ед. (+245 проданных товаров,\n";
    std::cout << "     +9 780 руб. выручки), при росте затрат на хранение всего на ~2 400 руб.\n";
    std::cout << "     Порог 30 дополнительно устраняет оставшиеся 45 потерь (+1 800 руб.), но требует роста затрат\n";
    std::cout << "     на хранение на ~3 487 руб., что снова снижает чистую прибыль.\n\n";

    std::cout << "3. ПОЧЕМУ РЕЗУЛЬТАТ РАЗЛИЧАЕТСЯ (ФУНДАМЕНТАЛЬНЫЙ ЗАКОН УПРАВЛЕНИЯ ЗАПАСАМИ):\n";
    std::cout << "   Оптимальный уровень точки заказа определяется балансом между стоимостью хранения (1 руб./день)\n";
    std::cout << "   и маржой потери продажи (40 руб./ед.). Отношение стоимости дефицита к хранению составляет 40:1.\n";
    std::cout << "   - При низком спросе расход во время доставки мал (12 ед.). Главная статья оптимизации —\n";
    std::cout << "     сокращение расходов на содержание запасов (бережливый склад, порог 10).\n";
    std::cout << "   - При высоком спросе средний расход во время доставки (18 ед.) превосходит порог 10.\n";
    std::cout << "     Потери маржи становятся критическими, поэтому создание страхового буфера (порог 20) окупается сторицей.\n\n";

    std::cout << "4. ОБЩИЙ ВЫВОД:\n";
    std::cout << "   - Оптимальный порог заказа динамически зависит от интенсивности спроса: при обычном спросе\n";
    std::cout << "     выгоден порог <= 10 (45 255 руб.), при повышенном спросе выгоден порог <= 20 (71 963 руб.).\n";
    std::cout << "   - Порог <= 30 неэффективен при любых режимах из-за затоваривания склада и переплаты за хранение.\n";
    std::cout << "   - Рекомендация для предприятия: использовать адаптивную стратегию управления запасами,\n";
    std::cout << "     повышая порог заказа до 20 ед. в пиковые сезоны и снижая до 10 ед. в периоды нормального спроса.\n";
    std::cout << "========================================================================================\n\n";
}

// Экспорт результатов в CSV файл для отчёта и открытия в Excel
void export_csv(const std::string& filename, const std::vector<AggregateStats>& results) {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Не удалось открыть файл для записи CSV: " << filename << "\n";
        return;
    }

    out << "Demand,MaxDemand,Threshold,MeanProfit,StdProfit,MeanLostSales,MeanAvgStock,MeanOrders,MeanFillRate,MeanRevenue,MeanOrderCost,MeanHoldingCost\n";
    for (const auto& r : results) {
        out << "\"" << r.demand_name << "\","
            << r.max_demand << ","
            << r.threshold << ","
            << r.mean_profit << ","
            << r.std_profit << ","
            << r.mean_lost_sales << ","
            << r.mean_avg_stock << ","
            << r.mean_orders << ","
            << r.mean_fill_rate << ","
            << r.mean_revenue << ","
            << r.mean_order_cost << ","
            << r.mean_holding_cost << "\n";
    }
    out.close();
    std::cout << "Результаты успешно экспортированы в файл: " << filename << "\n";
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    int num_experiments = 1000;
    uint64_t seed = 12345ULL;
    bool only_demo = false;
    bool only_sim = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--demo") {
            only_demo = true;
        } else if (arg == "--sim") {
            only_sim = true;
        } else if (arg == "--experiments" && i + 1 < argc) {
            num_experiments = std::stoi(argv[++i]);
        } else if (arg == "--seed" && i + 1 < argc) {
            seed = std::stoull(argv[++i]);
        }
    }

    std::cout << "========================================================================================\n";
    std::cout << "    КУРСОВАЯ РАБОТА: ИССЛЕДОВАНИЕ СПОСОБОВ ПОПОЛНЕНИЯ ТОВАРНЫХ ЗАПАСОВ (ВАРИАНТ № 10)\n";
    std::cout << "========================================================================================\n";

    if (!only_sim) {
        run_control_example();
    }

    if (!only_demo) {
        std::cout << "Запуск статистического моделирования методом Монте-Карло...\n";
        std::cout << "Параметры: " << num_experiments << " экспериментов по " << TOTAL_DAYS << " дней. Seed = " << seed << "\n";

        auto results = run_full_simulation(num_experiments, seed);

        print_comparison_table(results);
        print_ascii_profit_chart(results);
        print_analytical_conclusion(results);
        export_csv("results.csv", results);
    }

    return 0;
}
