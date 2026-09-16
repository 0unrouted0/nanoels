# Builds and runs the host tests for calibration_math.h.
# From the h4 folder: .\test\run_tests.ps1
#
# Uses Visual Studio's cl.exe if it is installed, and otherwise any g++ or clang++ on PATH - the
# tests are one self-contained translation unit with no dependencies, so nothing here cares which.
# Keeping both routes open matters because the machine that runs these is often the machine at the
# lathe, which is not necessarily the one with Visual Studio on it.

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$out = Join-Path $env:TEMP 'nanoels_tests'
New-Item -ItemType Directory -Force $out | Out-Null

$src = Join-Path $here 'test_calibration_math.cpp'
$exe = Join-Path $out 'test_calibration_math.exe'

$vcvars = Get-ChildItem @(
  'C:\Program Files\Microsoft Visual Studio',
  'C:\Program Files (x86)\Microsoft Visual C++ Build Tools',
  'C:\Program Files (x86)\Microsoft Visual Studio'
) -Recurse -Filter 'vcvars64.bat' -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName

if ($vcvars) {
  Write-Host "Compiler: cl.exe"
  $bat = Join-Path $out 'build.bat'
  # Via a batch file rather than an inline `cmd /c`, so paths with spaces quote cleanly.
  # Objects go to the working directory instead of /Fo: a quoted path ending in a backslash
  # reads as an escaped quote to cl and swallows the source filename after it.
  @(
    '@echo off'
    "call `"$vcvars`" >nul 2>&1"
    "cd /d `"$out`""
    "cl /nologo /EHsc /W4 /Fe:`"$exe`" `"$src`""
  ) | Set-Content -Path $bat -Encoding ASCII
  & $bat
} else {
  $gxx = Get-Command g++ -ErrorAction SilentlyContinue
  if (-not $gxx) { $gxx = Get-Command clang++ -ErrorAction SilentlyContinue }
  if (-not $gxx) {
    Write-Error 'No C++ compiler found. Install the Visual Studio C++ Build Tools, or a MinGW/LLVM toolchain such as: winget install --id BrechtSanders.WinLibs.POSIX.UCRT -e'
  }
  Write-Host "Compiler: $($gxx.Source)"
  # -Wall rather than /W4's nearest equivalent plus -Wextra: the table-driven tests index arrays
  # with signed loop counters throughout, and -Wextra's sign-compare noise buries real warnings.
  & $gxx.Source -std=c++17 -Wall -O1 -o $exe $src
}

if ($LASTEXITCODE -ne 0) { Write-Error 'Compilation failed.' }

& $exe
exit $LASTEXITCODE
