@echo off
setlocal enabledelayedexpansion
pushd "%~dp0..\Supplementary"

:: ---------------------------------------------------------------------------
:: 1) Isolate FA2sp.dll
::    FA2sp.dll and FA2sp.UnitTest.dll must not be loaded at the same time,
::    they hook the same addresses. A leftover "FA2sp.dll.isolated" from an
::    interrupted run is renamed back instead of being deleted: deleting it
::    would destroy FA2sp.dll for good.
:: ---------------------------------------------------------------------------
set /a ISOLATE_TRIES=0
:DO_ISOLATE
if exist "FA2sp.dll.isolated" (
    if not exist "FA2sp.dll" ren "FA2sp.dll.isolated" "FA2sp.dll" >nul 2>&1
    if exist "FA2sp.dll.isolated" del /f /q "FA2sp.dll.isolated" >nul 2>&1
)
if exist "FA2sp.dll" (
    ren "FA2sp.dll" "FA2sp.dll.isolated" >nul 2>&1
    if exist "FA2sp.dll" (
        set /a ISOLATE_TRIES+=1
        if !ISOLATE_TRIES! GEQ 15 (
            echo [RunUnitTest] ERROR: cannot isolate FA2sp.dll - close FinalAlert2 / stop the build and retry.
            popd
            exit /b 2
        )
        timeout /t 1 /nobreak >nul
        goto :DO_ISOLATE
    )
)

if exist "UnitTest.log" del /f /q "UnitTest.log" >nul 2>&1
if exist "UnitTest.xml" del /f /q "UnitTest.xml" >nul 2>&1

:: ---------------------------------------------------------------------------
:: 2) Output path workaround
::    GoogleTest 1.15 converts output file paths from UTF-8 (posix::FOpen)
::    while the working directory is obtained with the ANSI _getcwd(). When the
::    repository path contains non-ASCII characters (e.g. "...\???\..."), the
::    conversion throws inside std::wstring_convert and the test process dies
::    with exit code 3 before the XML report is written.
::    Let GoogleTest write to an ASCII-only path and copy the report back.
:: ---------------------------------------------------------------------------
set "GTEST_XML=%TEMP%\FA2spUnitTest.xml"
if exist "%GTEST_XML%" del /f /q "%GTEST_XML%" >nul 2>&1

set "ARG_HAS_OUTPUT="
set "ARG_LIST_TESTS="
echo(%*| findstr /C:"--gtest_output" >nul 2>&1 && set "ARG_HAS_OUTPUT=1"
echo(%*| findstr /C:"--gtest_list_tests" >nul 2>&1 && set "ARG_LIST_TESTS=1"

echo ========================================================
echo  Launching FA2sp Unit Tests via Syringe in FinalAlert2
echo ========================================================
if defined ARG_HAS_OUTPUT (
    Syringe.exe "FinalAlert2YR.dat" %*
) else (
    Syringe.exe "FinalAlert2YR.dat" --gtest_output=xml:%GTEST_XML% %*
)
set "SYRINGE_CODE=%ERRORLEVEL%"

if exist "%GTEST_XML%" copy /Y "%GTEST_XML%" "UnitTest.xml" >nul 2>&1

:: ---------------------------------------------------------------------------
:: 3) Restore FA2sp.dll
:: ---------------------------------------------------------------------------
set /a RESTORE_TRIES=0
:DO_RESTORE
if exist "FA2sp.dll.isolated" (
    ren "FA2sp.dll.isolated" "FA2sp.dll" >nul 2>&1
    if exist "FA2sp.dll.isolated" (
        set /a RESTORE_TRIES+=1
        if !RESTORE_TRIES! GEQ 15 (
            echo [RunUnitTest] WARNING: could not restore FA2sp.dll - it is still named FA2sp.dll.isolated.
            goto :JUDGE
        )
        timeout /t 1 /nobreak >nul
        goto :DO_RESTORE
    )
)

:JUDGE
:: ---------------------------------------------------------------------------
:: 4) Verdict
::    Syringe's exit code only reports whether the injection worked (it stays 0
::    even when the test process dies or tests fail), so judge by the reports.
:: ---------------------------------------------------------------------------
set "TEST_EXIT_CODE=0"
if not exist "UnitTest.log" (
    set "TEST_EXIT_CODE=1"
) else (
    findstr /C:"[  FAILED  ]" "UnitTest.log" >nul 2>&1 && set "TEST_EXIT_CODE=1"
    rem The final "[  PASSED  ]" line is only printed when the runner reached the
    rem end of the run. Without it the process died half way (e.g. the GoogleTest
    rem XML output bug above) - that must not count as success.
    if not defined ARG_LIST_TESTS findstr /C:"[  PASSED  ]" "UnitTest.log" >nul 2>&1 || set "TEST_EXIT_CODE=1"
)
if not defined ARG_HAS_OUTPUT if not defined ARG_LIST_TESTS if not exist "UnitTest.xml" set "TEST_EXIT_CODE=1"

if exist "UnitTest.log" (
    echo.
    type "UnitTest.log"
)
echo.
echo Syringe exit code: %SYRINGE_CODE% ^(injection status only^)
echo Tests exited with code: %TEST_EXIT_CODE%
popd
exit /b %TEST_EXIT_CODE%
