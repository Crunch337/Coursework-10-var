import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

# Set style
plt.rcParams['font.sans-serif'] = 'DejaVu Sans'
plt.rcParams['axes.edgecolor'] = '#cccccc'
plt.rcParams['axes.linewidth'] = 0.8

df = pd.read_csv('results.csv')

# Chart 1: Profit comparison
fig, ax = plt.subplots(figsize=(10, 6), dpi=300)

labels = [
    "Вар. 1: U[0, 8]\nПорог ≤ 10",
    "Вар. 2: U[0, 8]\nПорог ≤ 20",
    "Вар. 3: U[0, 8]\nПорог ≤ 30",
    "Вар. 4: U[0, 12]\nПорог ≤ 10",
    "Вар. 5: U[0, 12]\nПорог ≤ 20",
    "Вар. 6: U[0, 12]\nПорог ≤ 30"
]

profits = df['MeanProfit'].values
colors = ['#2b5c8f', '#41729f', '#588bae', '#d95f02', '#e7298a', '#7570b3']
# Use 2 palettes for the two demand levels
colors = ['#1f77b4', '#aec7e8', '#c6dbef', '#ff7f0e', '#2ca02c', '#d62728']
# Better: highlight the winner in each category
colors = ['#2e7d32', '#90caf9', '#bbdefb', '#ffb74d', '#1565c0', '#b39ddb']

bars = ax.bar(range(len(profits)), profits, color=colors, width=0.6, edgecolor='black', linewidth=0.8)

# Add values on top of bars
for bar in bars:
    yval = bar.get_height()
    ax.text(bar.get_x() + bar.get_width()/2.0, yval + 1000, f"{yval:,.1f} ₽".replace(',', ' '), ha='center', va='bottom', fontsize=10, fontweight='bold')

ax.set_xticks(range(len(labels)))
ax.set_xticklabels(labels, fontsize=10)
ax.set_ylabel('Средняя годовая прибыль (руб.)', fontsize=12)
ax.set_title('Сравнение средней годовой прибыли по 6 вариантам управления запасами\n(Вариант № 10: 1000 симуляций по 365 дней)', fontsize=13, fontweight='bold', pad=15)
ax.set_ylim(0, max(profits) * 1.15)
ax.grid(axis='y', linestyle='--', alpha=0.5)

# Annotate best results
ax.annotate('ОПТИМУМ при [0, 8]:\nПорог <= 10 ед.', 
            xy=(0, profits[0]), xytext=(0, profits[0] + 9000),
            arrowprops=dict(facecolor='black', shrink=0.08, width=1, headwidth=6),
            ha='center', fontsize=9, fontweight='bold', bbox=dict(boxstyle="round,pad=0.3", fc="#e8f5e9", ec="#2e7d32", lw=1.5))

ax.annotate('ОПТИМУМ при [0, 12]:\nПорог <= 20 ед.', 
            xy=(4, profits[4]), xytext=(4, profits[4] + 9000),
            arrowprops=dict(facecolor='black', shrink=0.08, width=1, headwidth=6),
            ha='center', fontsize=9, fontweight='bold', bbox=dict(boxstyle="round,pad=0.3", fc="#e3f2fd", ec="#1565c0", lw=1.5))

plt.tight_layout()
plt.savefig('profit_comparison.png', dpi=300)
plt.close()
print("Saved profit_comparison.png")

# Chart 2: 2x2 Multi-metric analysis
fig, axes = plt.subplots(2, 2, figsize=(14, 10), dpi=300)

demand_8 = df[df['MaxDemand'] == 8]
demand_12 = df[df['MaxDemand'] == 12]

x_indices = np.array([10, 20, 30])

# Subplot 1: Net Profit
ax = axes[0, 0]
ax.plot(x_indices, demand_8['MeanProfit'], marker='o', linewidth=2.5, color='#1f77b4', label='Обычный спрос [0, 8]')
ax.plot(x_indices, demand_12['MeanProfit'], marker='s', linewidth=2.5, color='#ff7f0e', label='Повышенный спрос [0, 12]')
ax.set_title('1. Чистая среднегодовая прибыль', fontsize=12, fontweight='bold')
ax.set_xlabel('Порог заказа (единиц)', fontsize=10)
ax.set_ylabel('Прибыль (руб.)', fontsize=10)
ax.set_xticks(x_indices)
ax.grid(True, linestyle='--', alpha=0.6)
ax.legend(fontsize=9)

# Subplot 2: Lost Sales
ax = axes[0, 1]
ax.plot(x_indices, demand_8['MeanLostSales'], marker='o', linewidth=2.5, color='#1f77b4', label='Обычный спрос [0, 8]')
ax.plot(x_indices, demand_12['MeanLostSales'], marker='s', linewidth=2.5, color='#ff7f0e', label='Повышенный спрос [0, 12]')
ax.set_title('2. Потерянные продажи за год', fontsize=12, fontweight='bold')
ax.set_xlabel('Порог заказа (единиц)', fontsize=10)
ax.set_ylabel('Потеряно (ед. товара)', fontsize=10)
ax.set_xticks(x_indices)
ax.grid(True, linestyle='--', alpha=0.6)
ax.legend(fontsize=9)

# Subplot 3: Holding Cost vs Order Cost
ax = axes[1, 0]
ax.plot(x_indices, demand_8['MeanHoldingCost'], marker='o', linewidth=2, linestyle='-', color='#1f77b4', label='Хранение [0, 8]')
ax.plot(x_indices, demand_12['MeanHoldingCost'], marker='s', linewidth=2, linestyle='-', color='#ff7f0e', label='Хранение [0, 12]')
ax.plot(x_indices, demand_8['MeanOrderCost'], marker='^', linewidth=1.5, linestyle=':', color='#1f77b4', alpha=0.7, label='Заказы [0, 8]')
ax.plot(x_indices, demand_12['MeanOrderCost'], marker='v', linewidth=1.5, linestyle=':', color='#ff7f0e', alpha=0.7, label='Заказы [0, 12]')
ax.set_title('3. Затраты на хранение и оформление заказов', fontsize=12, fontweight='bold')
ax.set_xlabel('Порог заказа (единиц)', fontsize=10)
ax.set_ylabel('Затраты (руб.)', fontsize=10)
ax.set_xticks(x_indices)
ax.grid(True, linestyle='--', alpha=0.6)
ax.legend(fontsize=8)

# Subplot 4: Average Warehouse Stock Level
ax = axes[1, 1]
ax.plot(x_indices, demand_8['MeanAvgStock'], marker='o', linewidth=2.5, color='#1f77b4', label='Обычный спрос [0, 8]')
ax.plot(x_indices, demand_12['MeanAvgStock'], marker='s', linewidth=2.5, color='#ff7f0e', label='Повышенный спрос [0, 12]')
ax.set_title('4. Средний остаток на складе', fontsize=12, fontweight='bold')
ax.set_xlabel('Порог заказа (единиц)', fontsize=10)
ax.set_ylabel('Средний запас (ед.)', fontsize=10)
ax.set_xticks(x_indices)
ax.grid(True, linestyle='--', alpha=0.6)
ax.legend(fontsize=9)

plt.suptitle('Комплексный анализ показателей управления запасами\n(Зависимость от порога заказа и интенсивности спроса)', fontsize=14, fontweight='bold', y=0.995)
plt.tight_layout()
plt.savefig('tradeoff_analysis.png', dpi=300)
plt.close()
print("Saved tradeoff_analysis.png")
