# Builds and runs the host tests for calibration_math.h.
# Needs Visual Studio's cl.exe; nothing else. From the h4 folder: .\test\run_tests.ps1

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$out = Join-Path $env:TEMP 'nanoels_tests'
New-Item -ItemType Directory -Force $out | Out-Null

$vcvars = Get-ChildItem @(
  'C:\Program Files\Microsoft Visual Studio',
  'C:\Program Files (x86)\Microsoft Visual Studio'
) -Recurse -Filter 'vcvars64.bat' -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName

if (-not $vcvars) {
  Write-Error 'vcvars64.bat not found - install Visual Studio or the C++ Build Tools.'
}

$src = Join-Path $here 'test_calibration_math.cpp'
$exe = Join-Path $out 'test_calibration_math.exe'
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
if ($LASTEXITCODE -ne 0) { Write-Error 'Compilation failed.' }

& $exe
exit $LASTEXITCODE
