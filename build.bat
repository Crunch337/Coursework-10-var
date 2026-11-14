@echo off
chcp 65001 > nul
echo Компиляция программы управления товарными запасами (Вариант 10)...
g++ -O3 -std=c++17 main.cpp -o store_sim.exe
if %ERRORLEVEL% EQU 0 (
    echo [УСПЕХ] Программа успешно скомпилирована: store_sim.exe
) else (
    echo [ОШИБКА] Сбой компиляции! Убедитесь, что g++ установлен и доступен в PATH.
)
