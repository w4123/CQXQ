. "$PSScriptRoot\helpers.ps1"

Set-Location $projectDir

# 检查必要命令

$cmake = Find-CMakeCommand

if (-Not $cmake)
{
    Write-Failure "请先安装 CMake"
    SafeExit 1
}

Write-Host "CMake 路径：$cmake"

# 检查是否生成

$configType = if ($args[0]) { $args[0] } else { "Debug" }

if (-Not (Test-Path ".\build\$configType\ALL_BUILD.vcxproj"))
{
    Write-Failure "请先运行 `"generate.ps1 $configType`" 来生成 CMake 构建目录"
    SafeExit 1
}

# CMake 构建

Write-Host "正在使用 CMake 构建项目……"

& $cmake --build .\build\$configType --config $configType -- "-p:Platform=x86"

if ($?)
{
    Write-Success "CMake 构建成功"
}
else
{
    Write-Failure "CMake 构建失败"
    SafeExit 1
}

# 退出

SafeExit 0
