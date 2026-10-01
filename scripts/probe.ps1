param([ValidateSet('Check','Flash')][string]$Action = 'Check')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$tools = Join-Path (Split-Path $root -Parent) 'mspm0-tools'
$openocd = Join-Path $tools 'openocd-new/openocd'
Push-Location $root
try {
    $arguments = @('-s', "$openocd/share/openocd/scripts", '-f', 'openocd/abrobot-mspm0g3507.cfg')
    if ($Action -eq 'Flash') {
        if (!(Test-Path 'build/init.out')) { throw 'Build first (Ctrl+Shift+B).' }
        $arguments += @('-c', 'program build/init.out verify reset exit')
    } else {
        $arguments += @('-c', 'init; targets; shutdown')
    }
    & "$openocd/bin/openocd.exe" @arguments
    if ($LASTEXITCODE -ne 0) { throw "OpenOCD failed ($LASTEXITCODE). Check USB, pairing, board power and SWD wiring." }
} finally { Pop-Location }
