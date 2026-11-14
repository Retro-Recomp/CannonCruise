@echo off
REM CannonCruise Build Script (Windows / VS 18 Community + CMake)
REM Guncelleme: 2026-11-14 - sistem CMake'i kullanir, taze build dizinine yazar.

echo ======================================
echo CannonCruise Build Script
echo ======================================
echo.

REM Yol tanimlari
set VS_ROOT=C:\Program Files\Microsoft Visual Studio\18\Community
set MSVC_VERSION=14.51.36231
set MSVC_BIN=%VS_ROOT%\VC\Tools\MSVC\%MSVC_VERSION%\bin\Hostx86\x86
set CMAKE_BIN=C:\Program Files\CMake\bin
set PROJECT_ROOT=C:\Users\mnkhprre\Documents\GitHub\CannonCruise
set CODE_DIR=%PROJECT_ROOT%\code
REM Taze build dizini: izlenen code\build_win32_v180 bayat (VS 17) cache icerir;
REM ona dokunulmaz. Gerekirse silip yeniden calistirin.
set BUILD_DIR=%CODE_DIR%\build_win32
REM NULL = headless (pencere yok); D3D9 = pencereli gercek render.
REM Goruntu icin D3D9 gerekli; NULL sadece log/headless test icindir.
set PLATFORM=D3D9

echo Proje koku: %PROJECT_ROOT%
echo Build dizini: %BUILD_DIR%
echo Platform: %PLATFORM%
echo.

echo [1/6] Environment ayarlaniyor...
set PATH=%MSVC_BIN%;%CMAKE_BIN%;%PATH%

echo [2/6] Araclar dogrulaniyor...
"%CMAKE_BIN%\cmake.exe" --version
if errorlevel 1 (
    echo HATA: CMake bulunamadi: "%CMAKE_BIN%\cmake.exe"
    pause
    exit /b 1
)

echo.
echo [3/6] Build dizini hazirlaniyor...
if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
    echo   OK: Build dizini olusturuldu: %BUILD_DIR%
) else (
    echo   Build dizini mevcut: %BUILD_DIR%
    echo   Not: "Visual Studio 17 2022" veya eski generator cache'i varsa
    echo   configure hata verir; dizini silip tekrar calistirin.
)

echo.
echo [4/6] CMake configure...
"%CMAKE_BIN%\cmake.exe" ^
    -S "%CODE_DIR%" ^
    -B "%BUILD_DIR%" ^
    -G "Visual Studio 18 2026" ^
    -A Win32 ^
    -DCANNONCRUISE_RW_PLATFORM=%PLATFORM%

if errorlevel 1 (
    echo HATA: CMake configure basarisiz!
    pause
    exit /b 1
)
echo   OK: CMake configure tamamlandi

echo.
echo [5/6] Derleme yapiliyor...
"%CMAKE_BIN%\cmake.exe" ^
    --build "%BUILD_DIR%" ^
    --config Release ^
    --parallel 8

if errorlevel 1 (
    echo HATA: Derleme basarisiz!
    pause
    exit /b 1
)
echo   OK: Derleme tamamlandi

echo.
echo [6/6] DLL kopyalama...
set DLL_SOURCE=%PROJECT_ROOT%\DLLs
set DLL_DEST=%BUILD_DIR%\Release
if exist "%DLL_SOURCE%" (
    if not exist "%DLL_DEST%" mkdir "%DLL_DEST%"
    copy "%DLL_SOURCE%\*" "%DLL_DEST%" /Y >nul
    echo   OK: DLL'ler kopyalandi
)

echo.
echo ======================================
echo BUILD BASARILI!
echo ======================================
echo.
echo Calistirmak icin:
echo   cd "%BUILD_DIR%\Release"
echo   CannonCruise.exe
echo.
echo Testler icin:
echo   "%CMAKE_BIN%\ctest.exe" --test-dir "%BUILD_DIR%" -C Release --output-on-failure
echo.

set /p RUN_TESTS="Testleri calistirmak ister misiniz? (E/H): "
if /i "%RUN_TESTS%"=="E" (
    "%CMAKE_BIN%\ctest.exe" --test-dir "%BUILD_DIR%" -C Release --output-on-failure
)

echo.
echo Build script tamamlandi.
pause
