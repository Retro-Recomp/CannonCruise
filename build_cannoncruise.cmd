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
if not exist "%DLL_SOURCE%\binkw32.dll" (
    echo HATA: Bink runtime bulunamadi: "%DLL_SOURCE%\binkw32.dll"
    pause
    exit /b 1
)
if not exist "%DLL_DEST%" mkdir "%DLL_DEST%"
REM Remove previously copied Windows DLLs so incremental builds stay clean.
for %%F in ("%DLL_SOURCE%\*.dll") do (
    if /I not "%%~nxF"=="binkw32.dll" (
        if exist "%DLL_DEST%\%%~nxF" (
            del /q "%DLL_DEST%\%%~nxF"
            if errorlevel 1 (
                echo HATA: Eski runtime DLL'i kaldirilamadi: "%%~nxF"
                pause
                exit /b 1
            )
        )
    )
)
copy "%DLL_SOURCE%\binkw32.dll" "%DLL_DEST%" /Y >nul
if errorlevel 1 (
    echo HATA: Bink runtime kopyalanamadi!
    pause
    exit /b 1
)
REM Copy only the app-owned Bink runtime. Bundling Windows DLLs from DLLs\
REM shadows system Direct3D/runtime DLLs and breaks D3D9 adapter discovery.
echo   OK: Yalnizca binkw32.dll kopyalandi; Windows DLL'leri sistemden yuklenir.

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
