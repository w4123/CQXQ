$originDir = Get-Location
$projectDir = Split-Path $PSScriptRoot -Parent

$vcpkgRoot = if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { "$projectDir\vcpkg" }
$vcpkgCmd = "$vcpkgRoot\vcpkg.exe"
$vcpkgTriplet = if ($env:VCPKG_TRIPLET) { $env:VCPKG_TRIPLET } else { "x86-windows-static-custom" }

# 辅助函数

function Write-Success
{
    param ($InputObject)
    Write-Host  "$InputObject" -ForegroundColor Green
}

function Write-Failure
{
    param ($InputObject)
    Write-Host  "$InputObject" -ForegroundColor Red
}

function SafeExit
{
    param ($ExitCode)
    Set-Location $originDir
    exit $ExitCode
}

function Find-CMakeCommand
{
    $cmd = Get-Command -Name cmake -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd }

    # try CMake built in with Visual Studio
    $vsInstances = Get-ChildItem "C:\Program Files (x86)\Microsoft Visual Studio" -Filter 20?? | Sort-Object -Descending
    foreach ($vs in $vsInstances)
    {
        $vsPath = $vs.Fullname
        $cmd = Get-ChildItem "$vsPath\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -ErrorAction SilentlyContinue
        if ($cmd) { return $cmd }
    }

    # try CMake downloaded by vcpkg
    $cmd = Get-ChildItem "$projectDir\vcpkg\downloads\tools\cmake-*\*\bin\cmake.exe" | Sort-Object -Descending
    if ($cmd) { return $cmd[0] }
}
