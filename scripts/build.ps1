param([switch]$Rebuild)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$vswhere = Join-Path ([Environment]::GetEnvironmentVariable('ProgramFiles(x86)')) 'Microsoft Visual Studio\Installer\vswhere.exe'
$msbuild = $null
if (Test-Path -LiteralPath $vswhere) {
    $msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
}
if (-not $msbuild) {
    $command = Get-Command MSBuild.exe -ErrorAction SilentlyContinue
    if ($command) { $msbuild = $command.Source }
}
if (-not $msbuild) { throw 'Install Visual Studio 2022 Build Tools with the C++ workload and v143 toolset.' }
$target = if ($Rebuild) { 'Rebuild' } else { 'Build' }
& $msbuild (Join-Path $projectRoot 'ContourStudio.sln') "/t:$target" /p:Configuration=Release /p:Platform=x64 /m /nologo
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Output (Join-Path $projectRoot 'build\Release\FREE_FOR_YOU.exe')