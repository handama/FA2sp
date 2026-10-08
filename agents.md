# Agent Guidelines & Development Standards

## 1. Mandatory Verification Rule
> [!IMPORTANT]
> **Always verify that the solution compiles with 0 errors and all unit tests PASS before submitting changes, finishing tasks, or committing code, unless the user explicitly requests to skip the verification step.**

## 2. Project Architecture & Context
- **Project**: FA2sp (FinalAlert 2 Special / Modernized Yuri's Revenge Map Editor extension).
- **Host Target**: `FinalAlert2YR.dat` (32-bit x86 Windows executable located in `Supplementary/`).
- **Platform**: Windows `Win32` (`x86`) only.
- **Toolchain**: Visual Studio 2022 (MSVC v143, C++20).
- **Submodules**:
  - `FA2pp` at `FA2pp/` (**CRITICAL**: Must remain 100% clean and untouched; do not commit into this submodule unless explicitly instructed).
  - `MFC42` at `MFC42/`.
  - `googletest` at `googletest/` (tag `v1.15.2`).
  - `lexilla`, `scintilla`.
- **Git Remote Constraints**:
  - **NEVER** push local commits to remote repositories (`git push`) unless the user explicitly requests it.

## 3. Unit Test (UT) Architecture
- **In-Process Test Architecture**:
  - Unit tests are located in `FA2sp.UnitTest/` and built into `Supplementary/FA2sp.UnitTest.dll`.
  - The test DLL hooks `CFinalSunApp::InitInstance` (address `0x41FAD0`) in `FinalAlert2YR.dat` via Syringe.
  - Upon entry, it redirects stdout/stderr to `Supplementary/UnitTest.log` and `Supplementary/UnitTest.xml`, forwards CLI arguments to GoogleTest (`testing::InitGoogleTest`), runs all tests via `RUN_ALL_TESTS()`, closes output streams, and terminates cleanly via `ExitProcess(res)` before any host GUI windows are created.
- **Strict Isolation Mechanism**:
  - During tests, `FA2sp.dll` must NOT be loaded alongside `FA2sp.UnitTest.dll` to prevent hook collisions.
  - The runner script `Supplementary/RunUnitTest.bat` automatically isolates `FA2sp.dll` (temporarily renaming it to `FA2sp.dll.isolated`) before launching Syringe, and restores it unconditionally upon test completion.

## 4. How to Build & Run Tests

### Build
From project root:
```bat
msbuild FA2sp.sln /m /p:Configuration=Release /p:Platform=x86
```
Or build the UnitTest project alone:
```bat
msbuild FA2sp.UnitTest/FA2sp.UnitTest.vcxproj /p:Configuration=Release /p:Platform=Win32 /m
```

### Run Unit Tests
From project root:
```bat
Scripts\RunUnitTest.bat
```
Or with GoogleTest Filters / Options:
```bat
Scripts\RunUnitTest.bat --gtest_filter=SequencedKeyListTest.*
Scripts\RunUnitTest.bat --gtest_filter=CINIOrderTrackerTest.*
Scripts\RunUnitTest.bat --gtest_repeat=5
```

### Inspect Test Results
- Console output: printed directly by `Scripts\RunUnitTest.bat`.
- Detailed log: `Supplementary/UnitTest.log`.
- XML report: `Supplementary/UnitTest.xml`.
- Injector log: `Supplementary/syringe.log`.
- Exit code: 0 indicates success, non-zero indicates test failure.

## 5. Local Release Simulation & Packaging
Package release zip archives for YR, MO, and RN locally using `Scripts/PackageRelease.ps1`:
```powershell
# Package all editions (YR, MO, RN) into dist/
./Scripts/PackageRelease.ps1

# Package a single edition with specific version:
./Scripts/PackageRelease.ps1 -Target RN -Version v1.6.4
```
