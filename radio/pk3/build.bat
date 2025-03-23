@echo off

python --version >nul 2>&1

if %errorlevel% neq 0 (
    echo Python 3 is not installed.
    exit /b
)

set RADIO_BASE_PK3_NAME=%1
set RADIO_EXTRA_PK3_NAME=%2

python .\pk3make.py .\build\radioracers.txt --force -o .\out\%RADIO_BASE_PK3_NAME% &
python .\pk3make.py .\build\radioracers_plus.txt --force -o .\out\%RADIO_EXTRA_PK3_NAME%

if exist .\out\%RADIO_BASE_PK3_NAME% (
    if exist .\out\%RADIO_EXTRA_PK3_NAME% (
        echo %RADIO_BASE_PK3_NAME% and %RADIO_EXTRA_PK3_NAME% were built successfully.
    ) else (
        echo %RADIO_EXTRA_PK3_NAME% wasn't built properly.
    )
) else (
    echo %RADIO_BASE_PK3_NAME% or %RADIO_EXTRA_PK3_NAME% wasn't built properly.
)