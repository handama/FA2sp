<#
.SYNOPSIS
    Packages FA2sp release zips for target editions (YR, MO, RN) both locally and in CI.

.PARAMETER Target
    The target edition to package: 'YR', 'MO', 'RN', or 'All'. Default is 'All'.

.PARAMETER Version
    The release version tag (e.g. 'v1.6.4'). Default is 'v1.6.4'.

.PARAMETER BinDir
    Directory containing compiled FA2sp.dll and FA2sp.pdb.
    If not specified, auto-detects from 'bin_output', 'Release', or 'Supplementary'.

.PARAMETER OutputDir
    Destination directory for output zip files. Default is 'dist'.

.PARAMETER SupplementaryDir
    Path to the Supplementary source directory. Default is project root's 'Supplementary'.

.EXAMPLE
    # Package all targets for local testing:
    ./Scripts/PackageRelease.ps1

.EXAMPLE
    # Package only RN edition with custom version:
    ./Scripts/PackageRelease.ps1 -Target RN -Version v1.6.4
#>

[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet("YR", "MO", "RN", "All")]
    [string]$Target = "All",

    [Parameter(Position = 1)]
    [string]$Version = "v1.6.4",

    [Parameter(Position = 2)]
    [string]$BinDir = "",

    [Parameter(Position = 3)]
    [string]$OutputDir = "dist",

    [Parameter()]
    [string]$SupplementaryDir = ""
)

$ErrorActionPreference = "Stop"

# Resolve project root relative to script location
$ProjectRoot = (Resolve-Path "$PSScriptRoot/..").Path

# Resolve Supplementary directory
if ([string]::IsNullOrWhiteSpace($SupplementaryDir)) {
    $SupplementaryDir = Join-Path $ProjectRoot "Supplementary"
}
if (-not (Test-Path $SupplementaryDir)) {
    throw "Supplementary directory not found: $SupplementaryDir"
}
$SupplementaryDir = (Resolve-Path $SupplementaryDir).Path

# Auto-detect BinDir if not provided
if ([string]::IsNullOrWhiteSpace($BinDir)) {
    $candidates = @(
        (Join-Path $ProjectRoot "bin_output"),
        (Join-Path $ProjectRoot "Release"),
        $SupplementaryDir
    )
    foreach ($cand in $candidates) {
        if (Test-Path (Join-Path $cand "FA2sp.dll")) {
            $BinDir = $cand
            break
        }
    }
}
if ([string]::IsNullOrWhiteSpace($BinDir) -or -not (Test-Path (Join-Path $BinDir "FA2sp.dll"))) {
    Write-Warning "FA2sp.dll not found in candidate binary paths. Packages will be built using files present in Supplementary."
} else {
    $BinDir = (Resolve-Path $BinDir).Path
    Write-Host "Using binary directory: $BinDir"
}

# Resolve Output directory
if (-not [System.IO.Path]::IsPathRooted($OutputDir)) {
    $OutputDir = [System.IO.Path]::GetFullPath((Join-Path (Get-Location) $OutputDir))
}
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

$targetsToProcess = if ($Target -eq "All") { @("YR", "MO", "RN") } else { @($Target) }

Write-Host "========================================================"
Write-Host " Packaging FA2sp ($Version) for targets: $($targetsToProcess -join ', ')"
Write-Host " Output Directory: $OutputDir"
Write-Host "========================================================"

foreach ($t in $targetsToProcess) {
    $pkgName = "FA2sp-$t-$Version.zip"
    $zipPath = Join-Path $OutputDir $pkgName
    $stageDir = Join-Path $OutputDir "staging_$t"

    Write-Host "`n--> Packaging target: $t"

    if (Test-Path $stageDir) {
        Remove-Item -Path $stageDir -Recurse -Force
    }

    # 1. Copy Supplementary base tree
    Copy-Item -Path $SupplementaryDir -Destination $stageDir -Recurse -Force

    # 2. Apply target modifications
    if ($t -eq "YR") {
        Remove-Item -Path "$stageDir/MentalOmega" -Recurse -Force -ErrorAction SilentlyContinue
        Remove-Item -Path "$stageDir/RevengeNow" -Recurse -Force -ErrorAction SilentlyContinue
    }
    elseif ($t -eq "MO") {
        if (Test-Path "$stageDir/MentalOmega") {
            Get-ChildItem -Path "$stageDir/MentalOmega" | ForEach-Object {
                Copy-Item -Path $_.FullName -Destination $stageDir -Recurse -Force
            }
        }
        Remove-Item -Path "$stageDir/MentalOmega" -Recurse -Force -ErrorAction SilentlyContinue
        Remove-Item -Path "$stageDir/RevengeNow" -Recurse -Force -ErrorAction SilentlyContinue
    }
    elseif ($t -eq "RN") {
        if (Test-Path "$stageDir/RevengeNow") {
            Get-ChildItem -Path "$stageDir/RevengeNow" | ForEach-Object {
                Copy-Item -Path $_.FullName -Destination $stageDir -Recurse -Force
            }
        }
        Remove-Item -Path "$stageDir/MentalOmega" -Recurse -Force -ErrorAction SilentlyContinue
        Remove-Item -Path "$stageDir/RevengeNow" -Recurse -Force -ErrorAction SilentlyContinue
    }

    # 3. Inject compiled binaries if available
    if (-not [string]::IsNullOrWhiteSpace($BinDir)) {
        $dllPath = Join-Path $BinDir "FA2sp.dll"
        $pdbPath = Join-Path $BinDir "FA2sp.pdb"
        if (Test-Path $dllPath) {
            Copy-Item -Force $dllPath "$stageDir/FA2sp.dll"
        }
        if (Test-Path $pdbPath) {
            Copy-Item -Force $pdbPath "$stageDir/FA2sp.pdb"
        }
    }

    # 4. Clean developer / test artifacts
    Remove-Item -Path "$stageDir/FA2sp.UnitTest.dll" -Force -ErrorAction SilentlyContinue
    Remove-Item -Path "$stageDir/RunUnitTest.bat" -Force -ErrorAction SilentlyContinue
    Remove-Item -Path "$stageDir/*.log" -Force -ErrorAction SilentlyContinue
    Remove-Item -Path "$stageDir/*.xml" -Force -ErrorAction SilentlyContinue
    Remove-Item -Path "$stageDir/*.isolated" -Force -ErrorAction SilentlyContinue

    # 5. Compress
    if (Test-Path $zipPath) {
        Remove-Item -Force $zipPath
    }
    Compress-Archive -Path "$stageDir/*" -DestinationPath $zipPath -Force

    # Cleanup staging directory
    Remove-Item -Path $stageDir -Recurse -Force

    $zipItem = Get-Item $zipPath
    Write-Host "Created: $pkgName ($([math]::Round($zipItem.Length / 1MB, 2)) MB)"
}

Write-Host "`nAll requested targets packaged successfully."
