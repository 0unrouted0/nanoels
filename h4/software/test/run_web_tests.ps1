# Runs the host tests for the JavaScript in web_page.h. Needs node.
#
# Separate from run_tests.ps1 on purpose: the C++ suite needs nothing but a compiler, and that
# should stay true. Skips rather than fails if node is missing, so it can be run unconditionally.
#
# From the h4 folder: .\test\run_web_tests.ps1

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path

$node = Get-Command node -ErrorAction SilentlyContinue
if (-not $node) {
  Write-Host 'node not found - skipping the web page tests.'
  Write-Host 'Install Node.js to run them; the C++ suite does not need it.'
  exit 0
}

& node (Join-Path $here 'test_web_page.js')
exit $LASTEXITCODE
