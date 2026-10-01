param([ValidateSet('Build','SysConfig')][string]$Action = 'Build')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$sdk = 'C:/TI/mspm0_sdk_2_11_00_07'
$compiler = 'D:/TI/ccs/tools/compiler/ti-cgt-armllvm_5.1.1.LTS'
$sysconfig = 'C:/TI/sysconfig_1.26.2'
$build = Join-Path $root 'build'
New-Item -ItemType Directory -Force -Path $build | Out-Null
function Invoke-Checked([string]$Executable, [string[]]$Arguments) {
    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Executable failed ($LASTEXITCODE)" }
}
if ($Action -eq 'SysConfig') {
    $gui = Join-Path (Split-Path $root -Parent) 'mspm0-tools/sysconfig_1.26.2'
    & 'D:/TI/SYSCONFIG/nw/nw.exe' "--user-data-dir=$gui/user-data" $gui --product "$sdk/.metadata/product.json" --script "$root/init.syscfg"
    return
}
Push-Location $build
try {
    Invoke-Checked "$sysconfig/sysconfig_cli.bat" @('--product', "$sdk/.metadata/product.json", '--script', "$root/init.syscfg", '--output', '.', '--compiler', 'ticlang')
    $cc = "$compiler/bin/tiarmclang.exe"
    $flags = @('@device.opt', '-march=thumbv6m', '-mcpu=cortex-m0plus', '-mfloat-abi=soft', '-mlittle-endian', '-mthumb', '-O2', '-g', '-Wall', "-I$root/init", "-I$root", "-I$build", "-I$sdk/source/third_party/CMSIS/Core/Include", "-I$sdk/source")
    $sources = @((Join-Path $root 'init.c')) + @(Get-ChildItem "$root/init" -Filter '*.c' -Recurse | Sort-Object FullName | ForEach-Object FullName) + @((Join-Path $build 'ti_msp_dl_config.c'), "$sdk/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c")
    $database = @()
    for ($sourceIndex = 0; $sourceIndex -lt $sources.Count; $sourceIndex++) {
        $source = $sources[$sourceIndex]
        $object = Join-Path $build (('{0:D2}_' -f $sourceIndex) + [IO.Path]::GetFileNameWithoutExtension($source) + '.o')
        $arguments = $flags + @('-c', $source, '-o', $object)
        $database += @{ directory=$build; file=$source; arguments=@($cc)+$arguments }
    }
    $database | ConvertTo-Json -Depth 6 | Set-Content "$root/compile_commands.json" -Encoding UTF8
    $objects = @()
    $index = 0
    foreach ($source in $sources) {
        $object = Join-Path $build (('{0:D2}_' -f $index) + [IO.Path]::GetFileNameWithoutExtension($source) + '.o')
        $arguments = $flags + @('-c', $source, '-o', $object)
        Write-Host "Compiling $([IO.Path]::GetFileName($source))"
        Invoke-Checked $cc $arguments
        $objects += $object
        $index++
    }
    $link = @('@device.opt', '-march=thumbv6m', '-mcpu=cortex-m0plus', '-mfloat-abi=soft', '-mthumb', '-g') + $objects + @('device_linker.cmd', '-Wl,-m,init.map', "-Wl,-i,$sdk/source", "-Wl,-i,$build", "-Wl,-i,$compiler/lib", '-Wl,--rom_model', '-Wl,--warn_sections', '-Wl,-ldevice.cmd.genlibs', '-Wl,-llibc.a', '-o', 'init.out')
    Invoke-Checked $cc $link
    Invoke-Checked "$compiler/bin/tiarmobjcopy.exe" @('-O', 'ihex', 'init.out', 'init.hex')
    Invoke-Checked "$compiler/bin/tiarmobjcopy.exe" @('-O', 'binary', 'init.out', 'init.bin')
    Write-Host "Build succeeded: $build/init.out (also .hex and .bin)"
} finally { Pop-Location }
