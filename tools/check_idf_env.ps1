<#
  check_idf_env.ps1
  Helper script to verify ESP-IDF environment readiness on Windows (PowerShell).

  - Checks presence of `export.ps1` in the repo root
  - Shows IDF related environment variables (e.g. IDF_PYTHON_ENV_PATH)
  - Attempts to import key Python modules and reports status
  - Prints suggested commands to fix common issues
#>
Param()

function Write-Title($s) { Write-Host "`n==> $s`n" -ForegroundColor Cyan }

Write-Title "Checking repository root for export.ps1"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$exportPath = Join-Path $root "export.ps1"
if (Test-Path $exportPath) {
  Write-Host "Found export script: $exportPath" -ForegroundColor Green
}
else {
  Write-Host "export.ps1 not found at: $exportPath" -ForegroundColor Yellow
  Write-Host "If you stored it elsewhere run it from its folder or provide the correct path." -ForegroundColor Yellow
}

Write-Title "Checking important environment variables"
$vars = @('IDF_PYTHON_ENV_PATH', 'IDF_PATH', 'PATH')
foreach ($v in $vars) {
  $val = [Environment]::GetEnvironmentVariable($v, 'Process')
  if ($val) { Write-Host "$v = $val" -ForegroundColor Green } else { Write-Host "$v is not set" -ForegroundColor Yellow }
}

Write-Title "Checking Python and required modules"
try {
  $python = (Get-Command python -ErrorAction Stop).Source
  Write-Host "python executable: $python" -ForegroundColor Green
  $code = @'
import importlib,sys
modules = ['esptool','esp_coredump']
for m in modules:
    try:
        mod = importlib.import_module(m)
        print(m+": OK")
    except Exception as e:
        print(m+": MISSING -> "+str(e))
'@
  $out = & python -c $code
  Write-Host $out
}
catch {
  Write-Host "Python not found in PATH or failed to run." -ForegroundColor Red
  Write-Host "Install Python 3.8+ and ensure it's available as 'python' in PATH." -ForegroundColor Yellow
}

Write-Title "Suggested fix commands (run in PowerShell from project root)"
Write-Host "1) From project root, run the export script (if present):" -ForegroundColor Cyan
Write-Host "export.ps1    # run to configure environment for IDF (execute in this shell or dot-source if you prefer)" -ForegroundColor White
Write-Host "2) Create/activate a venv and install IDF requirements if needed:" -ForegroundColor Cyan
Write-Host "python -m venv .venv" -ForegroundColor White
Write-Host ".\.venv\Scripts\Activate.ps1" -ForegroundColor White
Write-Host "pip install -r C:\Espressif\frameworks\esp-idf-v5.5.2\requirements.txt" -ForegroundColor White
Write-Host "3) Then run build:" -ForegroundColor Cyan
Write-Host "idf.py build" -ForegroundColor White

Write-Title "Notes"
Write-Host "If 'esptool' appears as an import error pointing to esp-idf's bundled esptool, remove conflicting packages from site-packages or use the IDF python env." -ForegroundColor Yellow
Write-Host "If you want, run this script from the project root to print diagnostics before building." -ForegroundColor Green

exit 0
