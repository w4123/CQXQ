. "$PSScriptRoot\helpers.ps1"

$appId = $args[0]
$libName = $args[1]
$outDir = $args[2]

$appOutDir = "$outDir\$appId"
New-Item -Path $appOutDir -ItemType Directory -ErrorAction SilentlyContinue

$dllName = "$libName.dll"
$dllOutPath = "$appOutDir\$libName.dll"

Copy-Item -Force $outDir\$dllName $dllOutPath

if (Test-Path "$PSScriptRoot\install.ps1")
{
    Write-Host "正在运行安装脚本 `"install.ps1 $args`"……"
    powershell.exe -ExecutionPolicy Bypass -NoProfile -File "$PSScriptRoot\install.ps1" $appId $libName $appOutDir
}

SafeExit 0
