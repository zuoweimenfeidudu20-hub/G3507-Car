param([ValidateSet('Check','CheckXds110','CheckWireless','Flash')][string]$Action = 'Check')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$tools = Join-Path (Split-Path $root -Parent) 'mspm0-tools'
$openocd = Join-Path $tools 'openocd-new/openocd'
Push-Location $root
try {
    if ($Action -eq 'CheckWireless') {
        $config = 'openocd/abrobot-mspm0g3507.cfg'
        $probeName = 'Wireless DAP'
    } else {
        $config = 'openocd/xds110-mspm0g3507.cfg'
        $probeName = 'XDS110'
    }
    $arguments = @('-s', "$openocd/share/openocd/scripts", '-f', $config)
    if ($Action -eq 'Flash') {
        if (!(Test-Path 'build/init.out')) { throw 'Build first (Ctrl+Shift+B).' }
        $arguments += @('-c', 'program build/init.out verify reset exit')
    } else {
        $arguments += @('-c', 'init; targets; shutdown')
    }
    & "$openocd/bin/openocd.exe" @arguments
    if ($LASTEXITCODE -ne 0) { throw "$probeName connection failed ($LASTEXITCODE). Check probe USB, board power and SWD wiring." }
} finally { Pop-Location }
