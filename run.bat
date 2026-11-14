@echo off
chcp 65001 > nul
if not exist store_sim.exe (
    echo Исполняемый файл store_sim.exe не найден. Выполняю сборку...
    call build.bat
)
echo Запуск моделирования...
store_sim.exe %*
