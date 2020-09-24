. "$PSScriptRoot\helpers.ps1"

Set-Location $projectDir

if (Test-Path .\build)
{
    Remove-Item -Recurse -Force -Path .\build
}

SafeExit 0
