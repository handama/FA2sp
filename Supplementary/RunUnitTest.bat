@echo off
pushd "%~dp0"

:DO_ISOLATE
if exist "FA2sp.dll" (
    if exist "FA2sp.dll.isolated" del /f /q "FA2sp.dll.isolated" >nul 2>&1
    ren "FA2sp.dll" "FA2sp.dll.isolated" >nul 2>&1
    if exist "FA2sp.dll" (
        timeout /t 1 /nobreak >nul
        goto :DO_ISOLATE
    )
)

if exist "UnitTest.log" del /f /q "UnitTest.log"
if exist "UnitTest.xml" del /f /q "UnitTest.xml"

echo ========================================================
echo  Launching FA2sp Unit Tests via Syringe in FinalAlert2
echo ========================================================
Syringe.exe "FinalAlert2YR.dat" %*
set TEST_EXIT_CODE=%ERRORLEVEL%

:DO_RESTORE
if exist "FA2sp.dll.isolated" (
    ren "FA2sp.dll.isolated" "FA2sp.dll" >nul 2>&1
    if exist "FA2sp.dll.isolated" (
        timeout /t 1 /nobreak >nul
        goto :DO_RESTORE
    )
)

if exist "UnitTest.log" (
    echo.
    type UnitTest.log
)
echo.
echo Tests exited with code: %TEST_EXIT_CODE%
popd
exit /b %TEST_EXIT_CODE%
